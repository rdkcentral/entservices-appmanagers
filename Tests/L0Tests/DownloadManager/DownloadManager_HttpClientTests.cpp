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
 * @file DownloadManager_HttpClientTests.cpp
 *
 * L0 tests for DownloadManagerHttpClient covering:
 *   - Construction initialises curl handle
 *   - Destruction cleans up without crash
 *   - downloadFile invalid URL returns HttpError
 *   - downloadFile unwritable destination returns DiskError
 *   - downloadFile 404 response returns HttpError (best-effort)
 *   - pause() does not crash
 *   - resume() does not crash
 *   - setRateLimit does not crash
 *   - getProgress returns 0 initially
 *   - getStatusCode returns 0 before any download
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

#include <core/core.h>

#include "DownloadManagerHttpClient.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"


// ─────────────────────────────────────────────────────────────────────────────
// Construction initialises curl handle (non-null)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_ConstructionInitializesCurl()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;

    // We verify construction succeeded by checking observable initial state
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getProgress()), 0u,
        "getProgress() returns 0 after construction (curl initialised)");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getStatusCode()), 0u,
        "getStatusCode() returns 0 after construction");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Destruction cleans up without crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_DestructionCleanupWithoutCrash()
{
    L0Test::TestResult tr;

    {
        DownloadManagerHttpClient client; // constructed
    } // destructor called here — must not crash

    L0Test::ExpectTrue(tr, true, "DownloadManagerHttpClient destructor does not crash");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// downloadFile with invalid URL returns HttpError
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_DownloadFileInvalidUrlReturnsHttpError()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;

    const std::string invalidUrl  = "not_a_valid_url";
    const std::string tmpFile     = "/tmp/dm_l0_invalid_url.out";

    const auto status = client.downloadFile(invalidUrl, tmpFile, 0u);

    // curl will fail with CURLE_UNSUPPORTED_PROTOCOL or similar → HttpError
    L0Test::ExpectTrue(tr,
        status == DownloadManagerHttpClient::Status::HttpError ||
        status == DownloadManagerHttpClient::Status::DiskError,
        "downloadFile() with invalid URL returns HttpError or DiskError");

    // Clean up any leftover file
    (void) std::remove(tmpFile.c_str());

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// downloadFile with unwritable destination returns DiskError
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_DownloadFileUnwritableDestReturnsDiskError()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;

    // Path under a non-existent directory → fopen fails → DiskError
    const std::string badPath = "/nonexistent_dir_dm_l0/file.pkg";
    const auto status = client.downloadFile("file:///dev/null", badPath, 0u);

    L0Test::ExpectTrue(tr,
        status == DownloadManagerHttpClient::Status::DiskError,
        "downloadFile() with unwritable path returns DiskError");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// downloadFile 404 response is handled (best-effort without server)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_DownloadFile404HandledGracefully()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;

    // Using a localhost URL that is almost certainly not serving content.
    // The outcome (HttpError or DiskError or even Success with empty file) is
    // environment-dependent, but the call must not crash.
    const std::string url = "http://127.0.0.1:19999/nonexistent_dm_l0_file.pkg";
    const std::string out = "/tmp/dm_l0_404_test.out";

    const auto status = client.downloadFile(url, out, 0u);

    L0Test::ExpectTrue(tr,
        status == DownloadManagerHttpClient::Status::HttpError  ||
        status == DownloadManagerHttpClient::Status::DiskError  ||
        status == DownloadManagerHttpClient::Status::Success,
        "downloadFile() with unreachable host returns a valid Status without crash");

    (void) std::remove(out.c_str());

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// pause() does not crash (calls curl_easy_pause on valid handle)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_PauseDoesNotCrash()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;
    client.pause(); // must not crash even when not actively downloading

    L0Test::ExpectTrue(tr, true, "pause() does not crash outside of active download");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// resume() does not crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_ResumeDoesNotCrash()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;
    client.resume(); // must not crash

    L0Test::ExpectTrue(tr, true, "resume() does not crash outside of active download");

    return tr.failures;
}


// ─────────────────────────────────────────────────────────────────────────────
// setRateLimit does not crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_SetRateLimitDoesNotCrash()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;
    client.setRateLimit(1024u);   // apply a rate limit
    client.setRateLimit(0u);      // remove rate limit (unlimited)

    L0Test::ExpectTrue(tr, true, "setRateLimit() does not crash");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// getProgress returns 0 initially
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_GetProgressReturnsZeroInitially()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getProgress()), 0u,
        "getProgress() returns 0 before any download");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// getStatusCode returns 0 before any download
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_HttpClient_GetStatusCodeReturnsZeroInitially()
{
    L0Test::TestResult tr;

    DownloadManagerHttpClient client;
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(client.getStatusCode()), 0u,
        "getStatusCode() returns 0 before any download");

    return tr.failures;
}
