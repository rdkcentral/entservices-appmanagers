#!/bin/bash
set -x
set -e
##############################
GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}
############################
# Build entservices-appmanagers
echo "building entservices-appmanagers"

cd ${GITHUB_WORKSPACE}
cmake -G Ninja -S "$GITHUB_WORKSPACE" -B build/entservices-appmanagers \
-DUSE_THUNDER_R4=ON \
-DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
-DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake" \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DCMAKE_DISABLE_FIND_PACKAGE_IARMBus=ON \
-DCMAKE_DISABLE_FIND_PACKAGE_RFC=ON \
-DCMAKE_DISABLE_FIND_PACKAGE_DS=ON \
-DCOMCAST_CONFIG=OFF \
-DRDK_SERVICES_COVERITY=ON \
-DRDK_SERVICES_L1_TEST=ON \
-DDS_FOUND=ON \
-DPLUGIN_LIFECYCLE_MANAGER=ON \
-DPLUGIN_APPMANAGER=ON \
-DPLUGIN_STORAGE_MANAGER=ON \
-DPLUGIN_PREINSTALL_MANAGER=ON \
-DPLUGIN_TELEMETRY_METRICS=ON \
-DPLUGIN_DOWNLOADMANAGER=ON \
-DPLUGIN_RUNTIME_MANAGER=ON \
-DPLUGIN_PACKAGE_MANAGER=ON \
-DCMAKE_CXX_FLAGS="-DEXCEPTIONS_ENABLE=ON \
-I ${GITHUB_WORKSPACE}/Tests/headers \
-I ${GITHUB_WORKSPACE}/Tests/headers/audiocapturemgr \
-I ${GITHUB_WORKSPACE}/Tests/headers/rdk/ds \
-I ${GITHUB_WORKSPACE}/Tests/headers/rdk/iarmbus \
-I ${GITHUB_WORKSPACE}/Tests/headers/rdk/iarmmgrs-hal \
-I ${GITHUB_WORKSPACE}/Tests/headers/ccec/drivers \
-I ${GITHUB_WORKSPACE}/Tests/headers/network \
-I ${GITHUB_WORKSPACE}/Tests/headers/libusb \
-I ${GITHUB_WORKSPACE}/Tests/headers/Dobby \
-I ${GITHUB_WORKSPACE}/Tests/headers/Dobby/Public/Dobby \
-I ${GITHUB_WORKSPACE}/Tests/headers/Dobby/IpcService \
-I ${GITHUB_WORKSPACE}/Tests/mocks \
-I ${GITHUB_WORKSPACE}/Tests/mocks/thunder \
-I /usr/include/libdrm \
-include ${GITHUB_WORKSPACE}/Tests/mocks/devicesettings.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/Iarm.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/Rfc.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/RBus.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/Telemetry.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/Udev.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/pkg.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/maintenanceMGR.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/secure_wrappermock.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/libusb/libusb.h \
-include ${GITHUB_WORKSPACE}/Tests/mocks/Dobby.h \
-include ${GITHUB_WORKSPACE}/Tests/headers/Dobby/DobbyProtocol.h \
-include ${GITHUB_WORKSPACE}/Tests/headers/Dobby/DobbyProxy.h \
-include ${GITHUB_WORKSPACE}/Tests/headers/Dobby/Public/Dobby/IDobbyProxy.h \
-include ${GITHUB_WORKSPACE}/Tests/headers/Dobby/IpcService/IpcFactory.h \
-Wall -Werror -Wno-error=format \
-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog \
-DENABLE_TELEMETRY_LOGGING -DUSE_IARMBUS \
-DENABLE_SYSTEM_GET_STORE_DEMO_LINK -DENABLE_DEEP_SLEEP \
-DENABLE_SET_WAKEUP_SRC_CONFIG -DENABLE_THERMAL_PROTECTION \
-DUSE_DRM_SCREENCAPTURE -DHAS_API_SYSTEM -DHAS_API_POWERSTATE \
-DHAS_RBUS -DDISABLE_SECURITY_TOKEN -DENABLE_DEVICE_MANUFACTURER_INFO -DUSE_THUNDER_R4=ON -DTHUNDER_VERSION=4 -DTHUNDER_VERSION_MAJOR=4 -DTHUNDER_VERSION_MINOR=4 -DENABLE_NATIVEBUILD=ON" \


cmake --build build/entservices-appmanagers --target install
echo "======================================================================================"
exit 0
