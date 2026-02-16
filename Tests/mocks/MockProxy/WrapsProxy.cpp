/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
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
*/

#include <stdarg.h>

extern "C" int __wrap_v_secure_system(const char *command, ...)
{
    // Need to implement the mockProxy for the wraps APIs
    return 0;
}

int v_secure_system(const char *command, va_list args)
{
    // Need to implement the mock for the OOP test, if need to verify the wraps APIs in unit test
    return 0;
}






