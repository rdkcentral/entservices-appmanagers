/* If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#include <cstdio>
#include <cstdlib>
#include <sys/shm.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <dirent.h>
#include "Module.h"
#include <core/core.h>
#include <plugins/Configuration.h>
#include "L2testController.h"

#ifndef CMAKE_INSTALL_PREFIX
#error CMAKE_INSTALL_PREFIX must be defined
#endif

#define L2TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#define THUNDER_ACCESS                  _T("THUNDER_ACCESS")            /* WPEFramework network address variable. */
#define THUNDER_ADDRESS                 "127.0.0.1"                     /* WPEFramework network address. */
#define THUNDER_ADDRESS_LENGTH           25                             /* WPEFramework network address length in bytes. */
#ifndef THUNDER_PORT
#define THUNDER_PORT                    "9998"                          /* Default WPEFramework port (as a string). */
#endif
#define TEST_CALLSIGN                   _T("thunder_tests.1")           /* Test module callsign. */
#define L2TEST_CALLSIGN                   _T("org.rdk.L2Tests.1")       /* L2 Test module callsign. */
#define THUNDER_CMD_LENGTH                 1024                          /* Command length to run thunder in isolation . */

#ifndef TEST_COMPLETION_TIMEOUT
#define TEST_COMPLETION_TIMEOUT                  600000                  /* Overall L2 Test completion timeout in milliseconds. */
#endif


#define FILESYSTEM_SYNC_DELAY_MS         100                            /* Delay for filesystem operations to complete (milliseconds). */
#define PLUGIN_INIT_DELAY_MS             500                            /* Delay for plugin initialization (milliseconds). */

#define EXIT_SUCCESS_CODE                0                              /* Exit code for successful execution. */
#define EXIT_AUTOSTART_FAILURE           1                              /* Exit code for autostart configuration failure. */
#define EXIT_THUNDER_START_FAILURE       2                              /* Exit code for Thunder startup failure. */
#define EXIT_TEST_EXECUTION_FAILURE      3                              /* Exit code for test execution failure. */

using namespace WPEFramework;
using namespace std;
// initializing instancePtr with NULL
L2testController* L2testController ::instancePtr = NULL;

L2testController* L2testController::getInstance()
{
  /*If there is no instance of class
   then we can create an instance */
  if (instancePtr == NULL)
  {
    instancePtr = new L2testController();
    return instancePtr;
  }
  else
  {
    /* if instancePtr != NULL that means
     the class already have an instance.
     So, we are returning that instance
     and not creating new one.*/
    return instancePtr;
  }
}
bool L2testController::StartThunder()
{
    char command[THUNDER_CMD_LENGTH];
    char address[THUNDER_ADDRESS_LENGTH];

    /* Spawn the Thunder process. */
#ifdef USE_THUNDER_R4
    snprintf(command, sizeof(command), "export GTEST_OUTPUT=\"json:$PWD/rdkL2TestResults.json\"; WPEFramework -c %s/../etc/WPEFramework/config.json -f", CMAKE_INSTALL_PREFIX);
#else
    snprintf(command, sizeof(command), "export GTEST_OUTPUT=\"json:$PWD/rdkL2TestResults.json\"; WPEFramework -c %s/../etc/WPEFramework/config.json", CMAKE_INSTALL_PREFIX);
#endif

    m_fp = popen(command, "w");
    if (nullptr == m_fp)
    {
        printf("Failed to start WPEFramework. Ensure that PATH and LD_LIRARY_PATH are set\n"
               "For example:\n"
               "$ export PATH=`pwd`/install/usr/bin:$PATH\n"
               "$ export LD_LIBRARY_PATH=`pwd`/install/usr/lib:$LD_LIBRARY_PATH\n");
        return false;
    }

    /* Allow time for the server to start. */
    (void)sleep(1U);

    /* Configure the JSONRPC connection, as the test starts before WPEFramework. */
    snprintf(address, sizeof(address), "%s:%s", THUNDER_ADDRESS, THUNDER_PORT);
    WPEFramework::Core::SystemInfo::SetEnvironment(THUNDER_ACCESS, address);

    return true;
}

void L2testController::StopThunder()
{

    if (nullptr != m_fp)
    {
        /* Terminate the Thunder process and wait for it to exit. */
        fprintf(m_fp, "Q\n");
        (void)pclose(m_fp);
        m_fp = nullptr;
    }

}

/**
 * @brief Perform L2 tests using the specified parameters
 *
 * @param[in] params The test parameters as a JSON string
 * @param[out] response The response string
 * @return Zero (Core::ERROR_NONE) on success or another value on error
 */
//---------------------------------------------
// Initialize COM-RPC client (with retries)
//---------------------------------------------
void L2testController::initClient() {
    if (m_client.IsValid() && m_l2TestInterface)
        return; // Already initialized

    const int MAX_RETRIES = 5;
    const int RETRY_DELAY_MS = 500;

    for (int attempt = 1; attempt <= MAX_RETRIES; ++attempt) {
        L2TEST_LOG("COM-RPC client initialization attempt #%d", attempt);

        m_client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"));
        if (!m_client.IsValid()) {
            L2TEST_LOG("Client creation failed, retrying...");
            usleep(RETRY_DELAY_MS * 1000);
            continue;
        }

        m_pluginShell = m_client->Open<PluginHost::IShell>(_T("org.rdk.L2Tests"), ~0, TEST_COMPLETION_TIMEOUT);
        if (!m_pluginShell) {
            L2TEST_LOG("Failed to open IShell, retrying...");
            m_client.Release();
            usleep(RETRY_DELAY_MS * 1000);
            continue;
        }

        m_l2TestInterface = m_pluginShell->QueryInterface<Exchange::IL2Test>();
        if (!m_l2TestInterface) {
            L2TEST_LOG("Failed to query IL2Test, retrying...");
            m_pluginShell->Release();
            m_pluginShell = nullptr;
            m_client.Release();
            usleep(RETRY_DELAY_MS * 1000);
            continue;
        }

        L2TEST_LOG("COM-RPC client initialized successfully");
        break;
    }

    if (!m_client.IsValid() || !m_l2TestInterface) {
        L2TEST_LOG("COM-RPC client initialization failed after retries");
    }
}

//---------------------------------------------
// Release COM-RPC client
//---------------------------------------------
void L2testController::releaseClient() {
    if (m_l2TestInterface) {
        m_l2TestInterface->Release();
        m_l2TestInterface = nullptr;
    }

    if (m_pluginShell) {
        m_pluginShell->Release();
        m_pluginShell = nullptr;
    }

    if (m_client.IsValid()) {
        m_client.Release();
    }
}

//---------------------------------------------
// Perform L2 tests
//---------------------------------------------
Core::hresult L2testController::PerformL2Tests(const std::string& params, std::string& response) {
    if (!m_client.IsValid() || !m_l2TestInterface) {
        L2TEST_LOG("COM-RPC client not initialized, initializing now...");
        initClient();
        if (!m_client.IsValid() || !m_l2TestInterface) {
            return Core::ERROR_GENERAL;
        }
    }

    return m_l2TestInterface->PerformL2Tests(params, response);
}

/**
 * @brief Find and replace a string in a file
 *
 * @param[in] fileName File name
 * @return Zero (EXIT_SUCCESS) on success or another value on error
 */
int find_replace_string(string fileName) {
    string filePath = "./install/etc/WPEFramework/plugins/";
    string fileFullName = filePath + fileName;
//    string fileFullName = "./install/etc/WPEFramework/plugins/test.json";
    string search = "\"autostart\":true";
    string replace = "\"autostart\":false";

    std::cout << "fileFullName: " << fileFullName << std::endl;
    // Open the file
    ifstream inputFile(fileFullName, ifstream::binary);
    if (!inputFile) {
        cout << "Error: Unable to open input file." << endl;
        return 1;
    }

    // Read the file into a string
    string content((istreambuf_iterator<char>(inputFile)), (istreambuf_iterator<char>()));
    inputFile.close();

    // Find and replace the string (search for both with and without space after colon)
    string search_with_space = "\"autostart\": true";
    size_t pos = content.find(search);
    bool replaced = false;
    while (pos != string::npos) {
        content.replace(pos, search.length(), replace);
        pos = content.find(search, pos + replace.length());
        replaced = true;
    }

    // Also search for pattern with space after colon
    pos = content.find(search_with_space);
    while (pos != string::npos) {
        content.replace(pos, search_with_space.length(), replace);
        pos = content.find(search_with_space, pos + replace.length());
        replaced = true;
    }

    // If no replacement was made, check if autostart field exists at all
    if (!replaced) {
        // Check if "autostart" keyword exists in the file
        if (content.find("\"autostart\"") == string::npos) {
            cout << "Info: No 'autostart' field found, adding it after 'callsign' in " << fileName << endl;

            // Find the "callsign" field to insert autostart after it
            size_t callsignPos = content.find("\"callsign\"");
            if (callsignPos != string::npos) {
                // Find the end of the callsign value (next comma or closing brace)
                size_t valueStart = content.find(':', callsignPos);
                if (valueStart != string::npos) {
                    // Skip the colon and any whitespace
                    valueStart++;
                    while (valueStart < content.length() && isspace(content[valueStart])) {
                        valueStart++;
                    }

                    // Find the end of the value (could be a string or other value)
                    size_t insertPos = valueStart;

                    // If it's a string value, find the closing quote
                    if (content[valueStart] == '"') {
                        insertPos = content.find('"', valueStart + 1);
                        if (insertPos != string::npos) {
                            insertPos++; // Move past the closing quote
                        }
                    } else {
                        // For non-string values, find the next comma or closing brace
                        size_t nextComma = content.find(',', valueStart);
                        size_t nextBrace = content.find('}', valueStart);
                        if (nextComma == string::npos && nextBrace == string::npos) {
                            insertPos = string::npos;
                        } else if (nextComma == string::npos) {
                            insertPos = nextBrace;
                        } else if (nextBrace == string::npos) {
                            insertPos = nextComma;
                        } else {
                            insertPos = (nextComma < nextBrace) ? nextComma : nextBrace;
                        }
                    }

                    if (insertPos != string::npos) {
                        // Insert autostart field after callsign
                        string insertion = ",\n \"autostart\": false";
                        content.insert(insertPos, insertion);
                        replaced = true;
                    } else {
                        cout << "Warning: Could not find end of callsign value" << endl;
                    }
                }
            } else {
                cout << "Warning: Could not find 'callsign' field in " << fileName << endl;
            }
        } else {
            cout << "Info: 'autostart' field exists but is already false in " << fileName << endl;
        }
    }

    // Open the file for writing
    ofstream outputFile(fileFullName);
    if (!outputFile) {
        cout << "Error: Unable to open output file." << endl;
        return 1;
    }

    // Write the modified content back to the file
    outputFile << content;
    outputFile.close();
    cout << "String replaced successfully." << endl;
    return 0;
}

/**

    * @brief Set autostart to false for all plugins
    * Some of the plugins are autostarted by default and when IARM calls are made in Initialize()
    * it will cause segmentation fault since the iarm mocks are not ready.
    * This function will set autostart to false for all plugins except L2Tests.json
    * @return Zero (EXIT_SUCCESS) on success or another value on error
    *
*/
int setAutostartToFalse()
{
    std::string path = "./install/etc/WPEFramework/plugins/";
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(path.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_REG) {
                std::cout << ent->d_name << std::endl;
                if (strcmp("L2Tests.json", ent->d_name) == 0)
                    continue;
                find_replace_string(ent->d_name);
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Error opening directory" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    std::string params;
    std::string response;
    Core::hresult status = Core::ERROR_GENERAL;
    int arguments=1;
    std::string message;
    L2testController* L2testobj = L2testController::getInstance();

    // Set autostart to false for all plugins except L2Tests.json
    L2TEST_LOG("Disabling autostart for plugins (except L2Tests)");
    int autostart_result = setAutostartToFalse();
    if (autostart_result != EXIT_SUCCESS)
    {
        L2TEST_LOG("Failed to disable autostart for plugins");
        return EXIT_AUTOSTART_FAILURE;
    }
    L2TEST_LOG("Autostart disabled successfully, waiting for file system sync");

    // Small delay to ensure filesystem operations complete and are visible
    usleep(FILESYSTEM_SYNC_DELAY_MS * 1000); // 100ms delay

    L2TEST_LOG("Starting Thunder");
    if (false == L2testobj->StartThunder())
    {
        L2TEST_LOG("Failed to start thunder");
        return EXIT_THUNDER_START_FAILURE;
    }

    // Give the plugin time to fully initialize
    usleep(PLUGIN_INIT_DELAY_MS * 1000); // 500ms delay

    L2TEST_LOG("Argument count = %d",argc);

    /* L2 tests can be run with different options
    ** RdkServicesL2Test  - To run all suits
    ** RdkServicesL2Test TestSuiteXXXX (seperated with space )
    ** Ex: RdkServicesL2Test Systemservice_L2test --> Runs only systemservice suite test
    **     RdkServicesL2Test Systemservice_L2test UsbAccess_L2test--> Runs systemservice and UsbAccesssuite tests
    **/
    if(argc > 1)  /* First argument is always executable */
    {
        message = std::string(argv[arguments]) + std::string("*");
        L2TEST_LOG("Argument passed = %s, message = %s",argv[arguments],message.c_str());
        arguments++;
        while(arguments<argc)
        {
            message = (message+std::string(":")+std::string(argv[arguments])+std::string("*"));
            L2TEST_LOG("Argument passed = %s",argv[arguments]);
            arguments++;
        }
        // Build JSON string for params
        JsonObject paramsJson;
        paramsJson["test_suite_list"] = message;
        paramsJson.ToString(params);
    }
    else
    {
        params = "{}";
        L2TEST_LOG("No Extra Argument passed");
    }

    L2TEST_LOG("PerformL2Tests from Test Controller");
    status = L2testobj->PerformL2Tests(params, response);

    if (status == Core::ERROR_NONE) {
        L2TEST_LOG("L2 tests completed successfully: %s", response.c_str());
    } else {
        L2TEST_LOG("L2 tests failed: %d, response: %s", status, response.c_str());
    }

    L2testobj->releaseClient();
    L2TEST_LOG("Stopping Thunder...");
    L2testobj->StopThunder();
    return (status == Core::ERROR_NONE ? EXIT_SUCCESS_CODE : EXIT_TEST_EXECUTION_FAILURE);

}
