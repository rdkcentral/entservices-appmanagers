#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

L1_ROOT="${REPO_ROOT}/l1build"
SRC_ROOT="${L1_ROOT}/src"
BUILD_ROOT="${L1_ROOT}/build"
APP_BUILD_DIR="${BUILD_ROOT}/entservices-appmanagers"
INSTALL_ROOT="${L1_ROOT}/install/usr"
BUILD_THREADS=4

resolve_jsoncpp_include() {
    local candidate

    if [[ -n "${JSONCPP_INCLUDE_DIR:-}" ]]; then
        if [[ -f "${JSONCPP_INCLUDE_DIR}/json/json.h" || -f "${JSONCPP_INCLUDE_DIR}/jsoncpp/json/json.h" ]]; then
            printf '%s' "${JSONCPP_INCLUDE_DIR}"
            return 0
        fi
    fi

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists jsoncpp; then
        while IFS= read -r candidate; do
            candidate="${candidate#-I}"
            if [[ -f "${candidate}/json/json.h" || -f "${candidate}/jsoncpp/json/json.h" ]]; then
                printf '%s' "${candidate}"
                return 0
            fi
        done < <(pkg-config --cflags-only-I jsoncpp | tr ' ' '\n' | sed '/^$/d')
    fi

    for candidate in \
        "${INSTALL_ROOT}/include/jsoncpp" \
        "${INSTALL_ROOT}/include" \
        "/usr/include/jsoncpp" \
        "/usr/local/include/jsoncpp" \
        "/usr/include" \
        "/usr/local/include"; do
        if [[ -f "${candidate}/json/json.h" || -f "${candidate}/jsoncpp/json/json.h" ]]; then
            printf '%s' "${candidate}"
            return 0
        fi
    done

    printf '%s' ""
}

resolve_jsoncpp_library() {
    local lib_path
    local libdir

    if [[ -n "${JSONCPP_LIBRARY:-}" && -f "${JSONCPP_LIBRARY}" ]]; then
        printf '%s' "${JSONCPP_LIBRARY}"
        return 0
    fi

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists jsoncpp; then
        libdir="$(pkg-config --variable=libdir jsoncpp 2>/dev/null || true)"
        if [[ -n "${libdir}" && -f "${libdir}/libjsoncpp.so" ]]; then
            printf '%s' "${libdir}/libjsoncpp.so"
            return 0
        fi
    fi

    lib_path="$(ldconfig -p 2>/dev/null | awk '/libjsoncpp\.so/{print $NF; exit}')"
    if [[ -n "${lib_path}" && -f "${lib_path}" ]]; then
        printf '%s' "${lib_path}"
        return 0
    fi

    for lib_path in \
        "${INSTALL_ROOT}/lib/libjsoncpp.so" \
        "/usr/lib/x86_64-linux-gnu/libjsoncpp.so" \
        "/usr/local/lib/libjsoncpp.so"; do
        if [[ -f "${lib_path}" ]]; then
            printf '%s' "${lib_path}"
            return 0
        fi
    done

    # Fallback for systems where only soname file exists (e.g. libjsoncpp.so.25)
    lib_path="$(find "${INSTALL_ROOT}/lib" /usr/lib/x86_64-linux-gnu /usr/local/lib -maxdepth 1 -type f -name 'libjsoncpp.so*' 2>/dev/null | head -n 1 || true)"
    if [[ -n "${lib_path}" && -f "${lib_path}" ]]; then
        printf '%s' "${lib_path}"
        return 0
    fi

    printf '%s' ""
}

if [[ ! -d "${APP_BUILD_DIR}" || ! -d "${INSTALL_ROOT}" ]]; then
    echo "ERROR: l1build baseline is missing. Run ./Tests/run_l1_from_l1build.sh first."
    exit 1
fi

if [[ ! -d "${SRC_ROOT}/Thunder" ]]; then
    echo "ERROR: Thunder source not found at ${SRC_ROOT}/Thunder"
    echo "Run ./Tests/run_l1_from_l1build.sh first."
    exit 1
fi

JSONCPP_INCLUDE_DIR_RESOLVED="$(resolve_jsoncpp_include)"
JSONCPP_LIBRARY_RESOLVED="$(resolve_jsoncpp_library)"

if [[ -z "${JSONCPP_INCLUDE_DIR_RESOLVED}" || -z "${JSONCPP_LIBRARY_RESOLVED}" ]]; then
    echo "ERROR: jsoncpp include/library not found."
    echo "Rerun ./Tests/run_l1_from_l1build.sh or export JSONCPP_INCLUDE_DIR/JSONCPP_LIBRARY."
    exit 1
fi

JSONCPP_LIB_DIR_RESOLVED="$(dirname "${JSONCPP_LIBRARY_RESOLVED}")"
mkdir -p "${INSTALL_ROOT}/lib"
if [[ ! -e "${INSTALL_ROOT}/lib/libjsoncpp.so" ]]; then
    ln -sf "${JSONCPP_LIBRARY_RESOLVED}" "${INSTALL_ROOT}/lib/libjsoncpp.so"
fi

LINKER_SEARCH_FLAGS="-L${INSTALL_ROOT}/lib -L${JSONCPP_LIB_DIR_RESOLVED}"

echo "[1/4] Reconfigure entservices-appmanagers for L1 (captures newly added tests)"
cmake -G Ninja \
    -S "${REPO_ROOT}" \
    -B "${APP_BUILD_DIR}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}" \
    -DCMAKE_PREFIX_PATH="${INSTALL_ROOT}" \
    -DCMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DCOMCAST_CONFIG=OFF \
    -DCMAKE_DISABLE_FIND_PACKAGE_DS=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_IARMBus=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_Udev=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_RFC=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_RBus=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DDS_FOUND=ON \
    -DHAS_FRONT_PANEL=ON \
    -DRDK_SERVICES_L1_TEST=ON \
    -DUSE_THUNDER_R4=ON \
    -DHIDE_NON_EXTERNAL_SYMBOLS=OFF \
    -DPLUGIN_APPMANAGER=ON \
    -DPLUGIN_APP_STORAGE_MANAGER=ON \
    -DPLUGIN_RUNTIME_MANAGER=OFF \
    -DPLUGIN_LIFECYCLE_MANAGER=OFF \
    -DPLUGIN_DOWNLOADMANAGER=OFF \
    -DPLUGIN_PREINSTALL_MANAGER=OFF \
    -DPLUGIN_RDK_WINDOW_MANAGER=OFF \
    -DPLUGIN_PACKAGE_MANAGER=OFF \
    -DENABLE_UNIT_TESTS=ON \
    -DJSONCPP_INCLUDE_DIR="${JSONCPP_INCLUDE_DIR_RESOLVED}" \
    -DJSONCPP_LIBRARY="${JSONCPP_LIBRARY_RESOLVED}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    -DCMAKE_MODULE_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -DEXCEPTIONS_ENABLE=ON -DUSE_THUNDER_R4=ON -DTHUNDER_VERSION=4 -DTHUNDER_VERSION_MAJOR=4 -DTHUNDER_VERSION_MINOR=4 -DRDK_SERVICES_L1_TEST -DBUILD_L1_TESTS_SHARED_MODULE=OFF -I ${REPO_ROOT}/Tests/headers -I ${REPO_ROOT}/Tests/headers/audiocapturemgr -I ${REPO_ROOT}/Tests/headers/rdk/ds -I ${REPO_ROOT}/Tests/headers/rdk/iarmbus -I ${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal -I ${REPO_ROOT}/Tests/headers/ccec/drivers -I ${REPO_ROOT}/Tests/headers/network -I ${REPO_ROOT}/Tests/headers/libusb -I ${REPO_ROOT}/Tests -I ${REPO_ROOT}/Tests/headers/Dobby -I ${REPO_ROOT}/Tests/headers/Dobby/Public/Dobby -I ${REPO_ROOT}/Tests/headers/Dobby/IpcService -I ${REPO_ROOT}/Tests/headers/rdkwindowmanager/include -I ${SRC_ROOT}/Thunder/Source -I ${SRC_ROOT}/Thunder/Source/core -I ${SRC_ROOT}/Thunder/Source/plugins -I ${INSTALL_ROOT}/include -I ${INSTALL_ROOT}/include/WPEFramework -I ${INSTALL_ROOT}/include/WPEFramework/plugins -include ${REPO_ROOT}/Tests/mocks/Iarm.h -include ${REPO_ROOT}/Tests/mocks/libusb.h -include ${REPO_ROOT}/Tests/mocks/Rfc.h -include ${REPO_ROOT}/Tests/mocks/RBus.h -include ${REPO_ROOT}/Tests/mocks/Telemetry.h -include ${REPO_ROOT}/Tests/mocks/Udev.h -include ${REPO_ROOT}/Tests/mocks/maintenanceMGR.h -include ${REPO_ROOT}/Tests/mocks/pkg.h -include ${REPO_ROOT}/Tests/mocks/secure_wrappermock.h -include ${REPO_ROOT}/Tests/mocks/wpa_ctrl_mock.h -include ${REPO_ROOT}/Tests/mocks/readprocMockInterface.h -include ${REPO_ROOT}/Tests/mocks/gdialservice.h -include ${REPO_ROOT}/Tests/mocks/RdkWindowManager.h --coverage -Wall -Wno-unused-result -Wno-deprecated-declarations -Wno-error=format= -Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog -Wl,-wrap,v_secure_system -Wl,-wrap,v_secure_popen -Wl,-wrap,v_secure_pclose -Wl,-wrap,unlink -DUSE_IARMBUS -DENABLE_SYSTEM_GET_STORE_DEMO_LINK -DENABLE_DEEP_SLEEP -DENABLE_SET_WAKEUP_SRC_CONFIG -DENABLE_THERMAL_PROTECTION -DDISABLE_SECURITY_TOKEN -DUSE_DRM_SCREENCAPTURE -DHAS_API_SYSTEM -DHAS_API_POWERSTATE -DHAS_RBUS -DENABLE_DEVICE_MANUFACTURER_INFO"

echo "[2/4] Rebuild changed code and L1 tests"
cmake --build "${APP_BUILD_DIR}" --parallel "${BUILD_THREADS}"

echo "[3/4] Reinstall updated binaries/libraries"
cmake --install "${APP_BUILD_DIR}"

echo "[4/4] Run RdkServicesL1Test with fresh result file"
export LD_LIBRARY_PATH="${INSTALL_ROOT}/lib:${INSTALL_ROOT}/lib/wpeframework/plugins:${LD_LIBRARY_PATH:-}"
export GTEST_OUTPUT="json:${L1_ROOT}/rdkL1TestResults.json"

TEST_BIN="${INSTALL_ROOT}/bin/RdkServicesL1Test"
if [[ ! -x "${TEST_BIN}" ]]; then
    TEST_BIN="$(find "${L1_ROOT}" -name RdkServicesL1Test -type f | head -n 1 || true)"
fi

if [[ -z "${TEST_BIN}" || ! -x "${TEST_BIN}" ]]; then
    echo "ERROR: RdkServicesL1Test binary not found"
    exit 1
fi

if [[ -n "${GTEST_FILTER:-}" ]]; then
    "${TEST_BIN}" --gtest_filter="${GTEST_FILTER}"
else
    "${TEST_BIN}"
fi

echo "Rebuild and L1 test run complete. Results: ${L1_ROOT}/rdkL1TestResults.json"
