#!/bin/bash
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2025 RDK Management
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

SCRIPT=$(readlink -f "$0")
SCRIPTS_DIR=`dirname "$SCRIPT"`
WORKSPACE=$SCRIPTS_DIR/../../../..

echo -e "${GREEN}========================================Run rdkservices===============================================${NC}"
cd $WORKSPACE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH/usr/local/lib/:$WORKSPACE/deps/rdk/hdmicec/install/lib:$WORKSPACE/deps/rdk/hdmicec/ccec/drivers/test:$WORKSPACE/deps/rdk/iarmbus/install/:$WORKSPACE/install/usr/lib:$WORKSPACE/deps/rdk/devicesettings/install/lib
$WORKSPACE/install/usr/bin/WPEFramework -f -c $WORKSPACE/install/etc/WPEFramework/config.json &