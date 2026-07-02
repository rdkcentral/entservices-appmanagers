/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

/**
 * Test-only replacement for RuntimeManager/ralf/RalfSupport.cpp.
 *
 * The only difference from the production file is the implementation of
 * JsonFromFile: this version opens the file with ::open() (a plain C syscall)
 * instead of std::ifstream.  Because ::open() is statically compiled into the
 * test binary it can be intercepted by the -Wl,--wrap,open linker mechanism,
 * enabling tests to simulate missing files without touching the real filesystem.
 *
 * std::ifstream (used in the production version) lives in libstdc++.so; its
 * internal open() calls go through that shared library's own PLT and therefore
 * bypass --wrap,open entirely.
 *
 * All other functions from RalfSupport.cpp are preserved verbatim: the original
 * source is included below with JsonFromFile temporarily renamed so there is no
 * duplicate-symbol conflict.
 */

// Rename the production JsonFromFile so it compiles under a private symbol name.
// parseRalPkgInfo (which calls JsonFromFile internally) becomes a private helper
// that uses std::ifstream; only RalfOCIConfigGenerator's calls to JsonFromFile
// go through the interceptable ::open()-based version defined at the bottom.
#define JsonFromFile JsonFromFile__real_impl_
#include "ralf/RalfSupport.cpp"
#undef JsonFromFile

#include <fcntl.h>  // O_RDONLY
#include <unistd.h>  // read, close
#include <sstream>   // std::istringstream

namespace ralf
{

/**
 * Interceptable replacement for JsonFromFile.
 * Opens the file via ::open() so -Wl,--wrap,open can be used in unit tests to
 * control whether the open succeeds.  The rest of the implementation (reading
 * the file content and parsing the JSON) is identical to the production version.
 */
bool JsonFromFile(const std::string &filePath, Json::Value &rootNode)
{
    // ::open() is compiled as a direct call site in this TU and is therefore
    // intercepted by __wrap_open when the test binary is linked.
    int fd = ::open(filePath.c_str(), O_RDONLY);
    if (fd < 0)
    {
        LOGERR("Failed to open JSON file: %s\n", filePath.c_str());
        return false;
    }

    std::string content;
    char buf[4096];
    ssize_t n;
    while ((n = ::read(fd, buf, sizeof(buf))) > 0)
        content.append(buf, static_cast<size_t>(n));
    ::close(fd);

    if (n < 0)
        return false;

    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream s(content);
    bool status = Json::parseFromStream(readerBuilder, s, &rootNode, &errs);
    if (!status)
        LOGERR("Failed to parse JSON: %s\n", errs.c_str());
    return status;
}

} // namespace ralf
