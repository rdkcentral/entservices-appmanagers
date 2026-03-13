#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

L1_ROOT="${REPO_ROOT}/l1build"
SRC_ROOT="${L1_ROOT}/src"
BUILD_ROOT="${L1_ROOT}/build"
INSTALL_ROOT="${L1_ROOT}/install/usr"

THUNDER_REF="R4.4.1"
THUNDERTOOLS_REF="R4.4.3"
ENTSERVICES_APIS_REF="develop"
GTEST_REF="v1.15.0"
JSONCPP_REF="1.9.5"

mkdir -p "${SRC_ROOT}" "${BUILD_ROOT}" "${INSTALL_ROOT}"

clone_if_missing() {
    local repo_url="$1"
    local target_dir="$2"
    local ref="$3"

    if [[ ! -d "${target_dir}/.git" ]]; then
        git clone --branch "${ref}" --depth 1 "${repo_url}" "${target_dir}"
    fi
}

apply_patch_once() {
    local target_dir="$1"
    local patch_file="$2"

    if [[ ! -f "${patch_file}" ]]; then
        echo "ERROR: Patch file not found: ${patch_file}"
        exit 1
    fi

    if patch -d "${target_dir}" -p1 --dry-run < "${patch_file}" >/dev/null 2>&1; then
        echo "Applying patch: $(basename "${patch_file}")"
        patch -d "${target_dir}" -p1 < "${patch_file}"
        return 0
    fi

    if patch -d "${target_dir}" -R -p1 --dry-run < "${patch_file}" >/dev/null 2>&1; then
        echo "Patch already applied, skipping: $(basename "${patch_file}")"
        return 0
    fi

    echo "WARNING: Patch cannot be applied cleanly (possibly drift/conflict): ${patch_file}"
}

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

ensure_jsoncpp() {
    JSONCPP_INCLUDE_DIR="$(resolve_jsoncpp_include)"
    JSONCPP_LIBRARY="$(resolve_jsoncpp_library)"

    if [[ -n "${JSONCPP_INCLUDE_DIR}" && -n "${JSONCPP_LIBRARY}" ]]; then
        return 0
    fi

    echo "jsoncpp not fully detected. Building local jsoncpp into l1build/install/usr..."
    clone_if_missing "https://github.com/open-source-parsers/jsoncpp.git" "${SRC_ROOT}/jsoncpp" "${JSONCPP_REF}"

    cmake -G Ninja \
        -S "${SRC_ROOT}/jsoncpp" \
        -B "${BUILD_ROOT}/jsoncpp" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DJSONCPP_WITH_TESTS=OFF \
        -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
        -DJSONCPP_WITH_PKGCONFIG_SUPPORT=ON \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}"
    cmake --build "${BUILD_ROOT}/jsoncpp" -j"$(nproc)"
    cmake --install "${BUILD_ROOT}/jsoncpp"

    JSONCPP_INCLUDE_DIR="$(resolve_jsoncpp_include)"
    JSONCPP_LIBRARY="$(resolve_jsoncpp_library)"

    if [[ -z "${JSONCPP_INCLUDE_DIR}" || -z "${JSONCPP_LIBRARY}" ]]; then
        echo "ERROR: Unable to resolve jsoncpp include/lib even after local build."
        echo "Set JSONCPP_INCLUDE_DIR and JSONCPP_LIBRARY environment variables and rerun."
        exit 1
    fi
}

ensure_jsoncpp
JSONCPP_LIB_DIR="$(dirname "${JSONCPP_LIBRARY}")"
mkdir -p "${INSTALL_ROOT}/lib"
if [[ ! -e "${INSTALL_ROOT}/lib/libjsoncpp.so" ]]; then
    ln -sf "${JSONCPP_LIBRARY}" "${INSTALL_ROOT}/lib/libjsoncpp.so"
fi
LINKER_SEARCH_FLAGS="-L${INSTALL_ROOT}/lib -L${JSONCPP_LIB_DIR}"
JSONCPP_CMAKE_ARGS=("-DJSONCPP_INCLUDE_DIR=${JSONCPP_INCLUDE_DIR}")
if [[ -n "${JSONCPP_LIBRARY}" ]]; then
    JSONCPP_CMAKE_ARGS+=("-DJSONCPP_LIBRARY=${JSONCPP_LIBRARY}")
fi

echo "[1/8] Cloning dependencies (no entservices-testframework checkout)"
clone_if_missing "https://github.com/rdkcentral/Thunder.git" "${SRC_ROOT}/Thunder" "${THUNDER_REF}"
clone_if_missing "https://github.com/rdkcentral/ThunderTools.git" "${SRC_ROOT}/ThunderTools" "${THUNDERTOOLS_REF}"
clone_if_missing "https://github.com/rdkcentral/entservices-apis.git" "${SRC_ROOT}/entservices-apis" "${ENTSERVICES_APIS_REF}"
clone_if_missing "https://github.com/google/googletest.git" "${SRC_ROOT}/googletest" "${GTEST_REF}"

echo "[2/8] Applying workflow patches"
apply_patch_once "${SRC_ROOT}/ThunderTools" "${REPO_ROOT}/Tests/patches/00010-R4.4-Add-support-for-project-dir.patch"

apply_patch_once "${SRC_ROOT}/Thunder" "${REPO_ROOT}/Tests/patches/Use_Legact_Alt_Based_On_ThunderTools_R4.4.3.patch"
apply_patch_once "${SRC_ROOT}/Thunder" "${REPO_ROOT}/Tests/patches/error_code_R4_4.patch"
apply_patch_once "${SRC_ROOT}/Thunder" "${REPO_ROOT}/Tests/patches/1004-Add-support-for-project-dir.patch"
apply_patch_once "${SRC_ROOT}/Thunder" "${REPO_ROOT}/Tests/patches/RDKEMW-733-Add-ENTOS-IDS.patch"
apply_patch_once "${SRC_ROOT}/Thunder" "${REPO_ROOT}/Tests/patches/Jsonrpc_dynamic_error_handling.patch"

apply_patch_once "${SRC_ROOT}/entservices-apis" "${REPO_ROOT}/Tests/patches/RDKEMW-1007.patch"

echo "[3/8] Building ThunderTools"
cmake -G Ninja \
    -S "${SRC_ROOT}/ThunderTools" \
    -B "${BUILD_ROOT}/ThunderTools" \
    -DEXCEPTIONS_ENABLE=ON \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}" \
    -DCMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake"
cmake --build "${BUILD_ROOT}/ThunderTools" -j"$(nproc)"
cmake --install "${BUILD_ROOT}/ThunderTools"

echo "[4/8] Building Thunder"
cmake -G Ninja \
    -S "${SRC_ROOT}/Thunder" \
    -B "${BUILD_ROOT}/Thunder" \
    -DMESSAGING=ON \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}" \
    -DCMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DBUILD_TYPE=Debug \
    -DBINDING=127.0.0.1 \
    -DPORT=55555 \
    -DEXCEPTIONS_ENABLE=ON
cmake --build "${BUILD_ROOT}/Thunder" -j"$(nproc)"
cmake --install "${BUILD_ROOT}/Thunder"

echo "[5/8] Building entservices-apis"
cmake -G Ninja \
    -S "${SRC_ROOT}/entservices-apis" \
    -B "${BUILD_ROOT}/entservices-apis" \
    -DEXCEPTIONS_ENABLE=ON \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}" \
    -DCMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake"
cmake --build "${BUILD_ROOT}/entservices-apis" -j"$(nproc)"
cmake --install "${BUILD_ROOT}/entservices-apis"

echo "[6/8] Building googletest"
cmake -G Ninja \
    -S "${SRC_ROOT}/googletest" \
    -B "${BUILD_ROOT}/googletest" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_ROOT}" \
    -DCMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="${INSTALL_ROOT}/../tools/cmake" \
    -DBUILD_TYPE=Debug \
    -DBUILD_GMOCK=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
cmake --build "${BUILD_ROOT}/googletest" -j"$(nproc)"
cmake --install "${BUILD_ROOT}/googletest"

echo "[7/8] Preparing generated external headers"
HEADER_DIRS=(
    "${REPO_ROOT}/Tests/headers/audiocapturemgr"
    "${REPO_ROOT}/Tests/headers/rdk/ds"
    "${REPO_ROOT}/Tests/headers/rdk/iarmbus"
    "${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal"
    "${REPO_ROOT}/Tests/headers/ccec/drivers"
    "${REPO_ROOT}/Tests/headers/network"
    "${REPO_ROOT}/Tests/headers/libusb"
    "${REPO_ROOT}/Tests/headers/Dobby/Public/Dobby"
    "${REPO_ROOT}/Tests/headers/Dobby/IpcService"
)

for header_dir in "${HEADER_DIRS[@]}"; do
    mkdir -p "${header_dir}"
done

HEADER_STUBS=(
    "${REPO_ROOT}/Tests/headers/rdk/ds/host.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/videoOutputPort.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/videoOutputPortType.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/videoOutputPortConfig.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/videoResolution.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/audioOutputPort.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/audioOutputPortType.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/audioOutputPortConfig.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/sleepMode.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/frontPanelConfig.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/frontPanelTextDisplay.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/hdmiIn.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/compositeIn.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/exception.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/dsError.h"
    "${REPO_ROOT}/Tests/headers/rdk/ds/dsMgr.h"
    "${REPO_ROOT}/Tests/headers/rdk/ds/manager.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/ds/dsTypes.h"
    "${REPO_ROOT}/Tests/headers/rdk/ds/dsUtl.h"
    "${REPO_ROOT}/Tests/headers/rdk/ds/pixelResolution.hpp"
    "${REPO_ROOT}/Tests/headers/rdk/iarmbus/libIARM.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmbus/libIBus.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmbus/libIBusDaemon.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmbus/iarmUtil.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal/mfrMgr.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal/pwrMgr.h"
    "${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal/sysMgr.h"
)

for header_stub in "${HEADER_STUBS[@]}"; do
    if [[ ! -e "${header_stub}" ]]; then
        : > "${header_stub}"
    fi
done

echo "[8/8] Building entservices-appmanagers L1 tests in l1build"
cmake -G Ninja \
    -S "${REPO_ROOT}" \
    -B "${BUILD_ROOT}/entservices-appmanagers" \
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
    -DPLUGIN_RUNTIME_MANAGER=ON \
    -DPLUGIN_LIFECYCLE_MANAGER=ON \
    -DPLUGIN_DOWNLOADMANAGER=ON \
    -DPLUGIN_PREINSTALL_MANAGER=ON \
    -DPLUGIN_PACKAGE_MANAGER=OFF \
    -DENABLE_UNIT_TESTS=ON \
    -DCMAKE_SHARED_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    -DCMAKE_MODULE_LINKER_FLAGS="${LINKER_SEARCH_FLAGS}" \
    "${JSONCPP_CMAKE_ARGS[@]}" \
    -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -DEXCEPTIONS_ENABLE=ON -DUSE_THUNDER_R4=ON -DTHUNDER_VERSION=4 -DTHUNDER_VERSION_MAJOR=4 -DTHUNDER_VERSION_MINOR=4 -DRDK_SERVICES_L1_TEST -DBUILD_L1_TESTS_SHARED_MODULE=OFF -I ${REPO_ROOT}/Tests/headers -I ${REPO_ROOT}/Tests/headers/audiocapturemgr -I ${REPO_ROOT}/Tests/headers/rdk/ds -I ${REPO_ROOT}/Tests/headers/rdk/iarmbus -I ${REPO_ROOT}/Tests/headers/rdk/iarmmgrs-hal -I ${REPO_ROOT}/Tests/headers/ccec/drivers -I ${REPO_ROOT}/Tests/headers/network -I ${REPO_ROOT}/Tests/headers/libusb -I ${REPO_ROOT}/Tests -I ${REPO_ROOT}/Tests/headers/Dobby -I ${REPO_ROOT}/Tests/headers/Dobby/Public/Dobby -I ${REPO_ROOT}/Tests/headers/Dobby/IpcService -I ${SRC_ROOT}/Thunder/Source -I ${SRC_ROOT}/Thunder/Source/core -I ${SRC_ROOT}/Thunder/Source/plugins -I ${INSTALL_ROOT}/include -I ${INSTALL_ROOT}/include/WPEFramework -I ${INSTALL_ROOT}/include/WPEFramework/plugins -include ${REPO_ROOT}/Tests/mocks/Iarm.h -include ${REPO_ROOT}/Tests/mocks/libusb.h -include ${REPO_ROOT}/Tests/mocks/Rfc.h -include ${REPO_ROOT}/Tests/mocks/RBus.h -include ${REPO_ROOT}/Tests/mocks/Telemetry.h -include ${REPO_ROOT}/Tests/mocks/Udev.h -include ${REPO_ROOT}/Tests/mocks/maintenanceMGR.h -include ${REPO_ROOT}/Tests/mocks/pkg.h -include ${REPO_ROOT}/Tests/mocks/secure_wrappermock.h -include ${REPO_ROOT}/Tests/mocks/wpa_ctrl_mock.h -include ${REPO_ROOT}/Tests/mocks/readprocMockInterface.h -include ${REPO_ROOT}/Tests/mocks/gdialservice.h --coverage -Wall -Wno-unused-result -Wno-deprecated-declarations -Wno-error=format= -Wl,-wrap,system -Wl,-wrap,popen -Wl,-wrap,syslog -Wl,-wrap,v_secure_system -Wl,-wrap,v_secure_popen -Wl,-wrap,v_secure_pclose -Wl,-wrap,unlink -DUSE_IARMBUS -DENABLE_SYSTEM_GET_STORE_DEMO_LINK -DENABLE_DEEP_SLEEP -DENABLE_SET_WAKEUP_SRC_CONFIG -DENABLE_THERMAL_PROTECTION -DDISABLE_SECURITY_TOKEN -DUSE_DRM_SCREENCAPTURE -DHAS_API_SYSTEM -DHAS_API_POWERSTATE -DHAS_RBUS -DENABLE_DEVICE_MANUFACTURER_INFO"

cmake --build "${BUILD_ROOT}/entservices-appmanagers" -j"$(nproc)"
cmake --install "${BUILD_ROOT}/entservices-appmanagers"

echo "Running RdkServicesL1Test"
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

"${TEST_BIN}"

echo "L1 tests complete. Results: ${L1_ROOT}/rdkL1TestResults.json"
