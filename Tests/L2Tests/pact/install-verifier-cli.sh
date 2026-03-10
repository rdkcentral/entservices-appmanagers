#!/usr/bin/env sh
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

set -e
set -x

VERSION="1.1.1"

mkdir -p ~/bin
case "$(uname -s)" in

   Darwin)
     echo '== Installing verifier CLI for Mac OSX =='
     if [ "$(uname -m)" == "arm64" ]; then
        curl -L -o ~/bin/pact_verifier_cli.gz https://github.com/pact-foundation/pact-reference/releases/download/pact_verifier_cli-v${VERSION}/pact_verifier_cli-osx-aarch64.gz
     else
        curl -L -o ~/bin/pact_verifier_cli.gz https://github.com/pact-foundation/pact-reference/releases/download/pact_verifier_cli-v${VERSION}/pact_verifier_cli-osx-x86_64.gz
     fi
     gunzip -N -f ~/bin/pact_verifier_cli.gz
     chmod +x ~/bin/pact_verifier_cli
     ;;

   Linux)
     echo '== Installing verifier CLI for Linux =='
     curl -L -o ~/bin/pact_verifier_cli.gz https://github.com/pact-foundation/pact-reference/releases/download/pact_verifier_cli-v${VERSION}/pact_verifier_cli-linux-x86_64.gz
     gunzip -N -f ~/bin/pact_verifier_cli.gz
     chmod +x ~/bin/pact_verifier_cli
     ;;

   CYGWIN*|MINGW32*|MSYS*|MINGW*)
     echo '== Installing verifier CLI for MS Windows =='
     curl -L -o ~/bin/pact_verifier_cli.exe.gz https://github.com/pact-foundation/pact-reference/releases/download/pact_verifier_cli-v${VERSION}/pact_verifier_cli-windows-x86_64.exe.gz
     gunzip -N -f ~/bin/pact_verifier_cli.exe.gz
     chmod +x ~/bin/pact_verifier_cli.exe
     ;;

   *)
     echo "ERROR: $(uname -s) is not a supported operating system"
     exit 1
     ;;
esac
