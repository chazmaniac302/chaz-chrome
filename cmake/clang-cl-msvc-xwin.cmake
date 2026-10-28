# Cross-compile toolchain: Linux host -> Windows x64 (MSVC ABI) using clang-cl + lld-link
# and the MSVC CRT / Windows SDK splatted by xwin (https://github.com/Jake-Shadle/xwin).
#   cmake -B build-win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-msvc-xwin.cmake -DXWIN_DIR=/path/to/xwin-sdk
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT XWIN_DIR)
    if(DEFINED ENV{XWIN_DIR})
        set(XWIN_DIR "$ENV{XWIN_DIR}")
    else()
        set(XWIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../tools/xwin-sdk")
    endif()
endif()
get_filename_component(XWIN_DIR "${XWIN_DIR}" ABSOLUTE)
set(XWIN_DIR "${XWIN_DIR}" CACHE PATH "xwin splat output directory")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XWIN_DIR)

find_program(CLANG_CL NAMES clang-cl clang-cl-19 clang-cl-18 clang-cl-17 PATHS /usr/lib/llvm-19/bin REQUIRED)
find_program(LLD_LINK NAMES lld-link lld-link-19 REQUIRED)
find_program(LLVM_RC  NAMES llvm-rc llvm-rc-19 REQUIRED)
find_program(LLVM_LIB NAMES llvm-lib llvm-lib-19 PATHS /usr/lib/llvm-19/bin REQUIRED)
find_program(LLVM_MT  NAMES llvm-mt llvm-mt-19 REQUIRED)

set(CMAKE_C_COMPILER   "${CLANG_CL}")
set(CMAKE_CXX_COMPILER "${CLANG_CL}")
set(CMAKE_LINKER       "${LLD_LINK}")
set(CMAKE_AR           "${LLVM_LIB}")
set(CMAKE_RC_COMPILER  "${LLVM_RC}")
set(CMAKE_MT           "${LLVM_MT}")
set(CMAKE_C_COMPILER_TARGET   x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

set(_xwin_inc "/imsvc\"${XWIN_DIR}/crt/include\" /imsvc\"${XWIN_DIR}/sdk/include/ucrt\" /imsvc\"${XWIN_DIR}/sdk/include/um\" /imsvc\"${XWIN_DIR}/sdk/include/shared\" /imsvc\"${XWIN_DIR}/sdk/include/winrt\"")
set(CMAKE_C_FLAGS_INIT   "--target=x86_64-pc-windows-msvc -fms-compatibility-version=19.40 ${_xwin_inc}")
set(CMAKE_CXX_FLAGS_INIT "--target=x86_64-pc-windows-msvc -fms-compatibility-version=19.40 /EHsc ${_xwin_inc}")
# llvm-rc needs the SDK headers too (JUCE's version resource includes winver.h)
set(CMAKE_RC_FLAGS_INIT "/I \"${XWIN_DIR}/sdk/include/um\" /I \"${XWIN_DIR}/sdk/include/shared\" /I \"${XWIN_DIR}/crt/include\" /I \"${XWIN_DIR}/sdk/include/ucrt\"")


# The SDK import libraries are lower-case on disk, but JUCE's #pragma comment(lib, ...) uses mixed
# case (D2d1.lib, DbgHelp.lib, ...). Linux file systems are case-sensitive, so add symlinks.
set(_xwin_case_dir "${XWIN_DIR}/libcase")
if(NOT EXISTS "${_xwin_case_dir}/.done")
    file(MAKE_DIRECTORY "${_xwin_case_dir}")
    foreach(_l D2d1 D3D11 DComp DXGI DbgHelp Dwrite GlU32 OpenGL32 Shell32 Strmiids)
        string(TOLOWER "${_l}" _lower)
        foreach(_d "${XWIN_DIR}/sdk/lib/um/x86_64" "${XWIN_DIR}/crt/lib/x86_64")
            if(EXISTS "${_d}/${_lower}.lib")
                file(CREATE_LINK "${_d}/${_lower}.lib" "${_xwin_case_dir}/${_l}.lib" SYMBOLIC)
                file(CREATE_LINK "${_d}/${_lower}.lib" "${_xwin_case_dir}/${_l}.Lib" SYMBOLIC)
            endif()
        endforeach()
    endforeach()
    file(WRITE "${_xwin_case_dir}/.done" "")
endif()

set(_xwin_lib "/libpath:\"${_xwin_case_dir}\" /libpath:\"${XWIN_DIR}/crt/lib/x86_64\" /libpath:\"${XWIN_DIR}/sdk/lib/um/x86_64\" /libpath:\"${XWIN_DIR}/sdk/lib/ucrt/x86_64\"")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_xwin_lib}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_xwin_lib}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_xwin_lib}")

set(CMAKE_TRY_COMPILE_CONFIGURATION Release)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
