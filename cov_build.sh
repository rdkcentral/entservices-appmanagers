#!/bin/bash
set -x
set -e
##############################
export PATH=/usr/local/bin:$PATH
cmake --version

GITHUB_WORKSPACE="${PWD}"
ls -la ${GITHUB_WORKSPACE}

# Exclude non-product directories from the build source tree so coverity/scans
# are run against a clean product build without those folders.
# Important: the L1 test CMake files hardcode ../entservices-appmanagers, so the
# filtered source tree must live as a sibling of the repo root, not inside it and
# not in /tmp, otherwise those generated include paths resolve incorrectly.
REPO_PARENT="$(dirname "${GITHUB_WORKSPACE}")"
COV_BUILD_ROOT="${REPO_PARENT}/entservices-appmanagers-cov"
COV_BUILD_SOURCE="${COV_BUILD_ROOT}/entservices-appmanagers"
rm -rf "${COV_BUILD_ROOT}"
mkdir -p "${COV_BUILD_SOURCE}"

echo "DEBUG: repo root: ${GITHUB_WORKSPACE}"
echo "DEBUG: sibling scan-prep tree: ${COV_BUILD_SOURCE}"
find "${GITHUB_WORKSPACE}" -mindepth 1 -maxdepth 1 \
    ! -name '.git' \
    ! -name '.cov_build_source' \
    ! -name 'build' \
    ! -name 'tests' \
    ! -name 'libocispec' \
    ! -name 'develop' \
    ! -name 'openspec' \
    -exec cp -a {} "${COV_BUILD_SOURCE}/" \;

echo "DEBUG: excluded directories: .git .cov_build_source build tests libocispec develop openspec; keeping install and Tests required by build"

# Native/L1 build environment: ensure AppManager sees jsoncpp headers via its env-based include hook.
export APP_MANAGER_INCLUDES="/usr/include/jsoncpp"

############################
# Build entservices-appmanagers
echo "building entservices-appmanagers"

cd "${COV_BUILD_SOURCE}"
PREFIX_PATH="${CMAKE_PREFIX_PATH:+${CMAKE_PREFIX_PATH};}${COV_BUILD_SOURCE}/install/usr;${COV_BUILD_SOURCE}/eshelpers;/usr"

# Coverity workflow only: make CompileSettingsDebug export symbols for direct test linking.
COMPILE_SETTINGS_DIR="${COV_BUILD_SOURCE}/install/usr/lib/cmake/CompileSettingsDebug"
if [ -d "${COMPILE_SETTINGS_DIR}" ]; then
	find "${COMPILE_SETTINGS_DIR}" -type f -name "*.cmake" | while read -r cmake_file; do
		perl -pi -e 's/-fvisibility=hidden/-fvisibility=default/g' "${cmake_file}"
		perl -pi -e 's/[ \t]*-fvisibility-inlines-hidden[ \t]*/ /g' "${cmake_file}"
	done
	COMPILE_SETTINGS_DEBUG_ARG="-DCompileSettingsDebug_DIR=${COMPILE_SETTINGS_DIR}"
else
	COMPILE_SETTINGS_DEBUG_ARG=""
fi

cmake -G Ninja -S "${COV_BUILD_SOURCE}" -B build/entservices-appmanagers \
-DUSE_THUNDER_R4=ON \
-DCMAKE_INSTALL_PREFIX="${COV_BUILD_SOURCE}/install/usr" \
-DCMAKE_MODULE_PATH="${COV_BUILD_SOURCE}/install/tools/cmake" \
-DCMAKE_PREFIX_PATH="${PREFIX_PATH}" \
${COMPILE_SETTINGS_DEBUG_ARG:+${COMPILE_SETTINGS_DEBUG_ARG}} \
-DCMAKE_DISABLE_FIND_PACKAGE_IARMBus=ON \
-DCMAKE_DISABLE_FIND_PACKAGE_RFC=ON \
-DCOMCAST_CONFIG=OFF \
-DRDK_SERVICES_COVERITY=ON \
-DRDK_SERVICES_L1_TEST=ON \
-DPLUGIN_LIFECYCLE_MANAGER=ON \
-DPLUGIN_APPMANAGER=ON \
-DPLUGIN_APP_STORAGE_MANAGER=ON \
-DPLUGIN_PREINSTALL_MANAGER=ON \
-DPLUGIN_TELEMETRY_METRICS=ON \
-DPLUGIN_DOWNLOADMANAGER=OFF \
-DPLUGIN_RUNTIME_MANAGER=ON \
-DPLUGIN_VICTIM_SELECTOR=ON \
-DPLUGIN_PACKAGE_MANAGER=OFF \
-DCMAKE_CXX_FLAGS="-fvisibility=default -DEXCEPTIONS_ENABLE=ON \
-I ${COV_BUILD_SOURCE}/Tests/mocks \
-I ${COV_BUILD_SOURCE}/Tests/mocks/thunder \
-I ${COV_BUILD_SOURCE}/helpers/Telemetry \
-include ${COV_BUILD_SOURCE}/Tests/mocks/Iarm.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/Rfc.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/RBus.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/Telemetry.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/Udev.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/pkg.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/maintenanceMGR.h \
-include ${COV_BUILD_SOURCE}/Tests/mocks/secure_wrappermock.h \
-Wall -Werror -Wno-error=format \
-Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog \
-DENABLE_TELEMETRY_LOGGING -DUSE_IARMBUS \
-DENABLE_SYSTEM_GET_STORE_DEMO_LINK -DENABLE_DEEP_SLEEP \
-DENABLE_SET_WAKEUP_SRC_CONFIG -DENABLE_THERMAL_PROTECTION \
-DUSE_DRM_SCREENCAPTURE -DHAS_API_SYSTEM -DHAS_API_POWERSTATE \
-DHAS_RBUS -DDISABLE_SECURITY_TOKEN -DENABLE_DEVICE_MANUFACTURER_INFO -DUSE_THUNDER_R4 -DTHUNDER_VERSION=4 -DTHUNDER_VERSION_MAJOR=4 -DTHUNDER_VERSION_MINOR=4 -DENABLE_NATIVEBUILD=ON" \



# Ensure generated Ninja compile rules cannot force hidden visibility.
BUILD_DIR="${COV_BUILD_SOURCE}/build/entservices-appmanagers"
if [ -d "${BUILD_DIR}" ]; then
	find "${BUILD_DIR}" -type f \( -name "*.ninja" -o -name "flags.make" \) | while read -r build_file; do
		perl -pi -e 's/-fvisibility=hidden/-fvisibility=default/g' "${build_file}"
		perl -pi -e 's/[ \t]*-fvisibility-inlines-hidden[ \t]*/ /g' "${build_file}"
	done
fi

cmake --build build/entservices-appmanagers --target install
echo "======================================================================================"
exit 0

