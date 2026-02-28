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

#include "RfcMock.h"

WDMP_STATUS getRFCParameter(const char* arg1, const char* arg2, RFC_ParamData_t* arg3)
{
    return RfcMock::Mock().getRFCParameter(arg1, arg2, arg3);
}

WDMP_STATUS setRFCParameter(const char* arg1, const char* arg2, const char* arg3, DATA_TYPE arg4)
{
    return RfcMock::Mock().setRFCParameter(arg1, arg2, arg3, arg4);
}

const char* getRFCErrorString(WDMP_STATUS arg1)
{
    return RfcMock::Mock().getRFCErrorString(arg1);
}
