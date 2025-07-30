cmake_minimum_required(VERSION 3.6)
project(nfcd)

# xHook target
file(GLOB XH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/xHook/libxhook/jni/*.c)
set(XH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/xHook/libxhook/jni/)
add_library(xh STATIC ${XH_SOURCES})
target_include_directories(xh PUBLIC ${XH_INCLUDE_DIR})

# find files
file(GLOB_RECURSE NFCD_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE NFCD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# cxx standard
set(CMAKE_CXX_STANDARD 17)

# compile library
add_library(nfcgate SHARED ${NFCD_HEADERS} ${NFCD_SOURCES})

# platform specific flags
if (CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    # FIXME XOM removed in NDK versions >= 22, retaining pre-22 behavior for compatibility
    if (ANDROID_NDK_REVISION VERSION_GREATER 21)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld -ffixed-x18")
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-execute-only -fuse-ld=lld -execute-only -ffixed-x18")
    endif()
endif()

# 16KB page size support - ensure compatibility with both 4KB and 16KB devices
if (CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    # Set maximum page size to 16KB for arm64-v8a
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,max-page-size=16384")
endif()

# link library
target_link_libraries(nfcgate android log xh)
# include directory
target_include_directories(nfcgate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jni/include/)

# for reproducible build
target_link_options(nfcgate PRIVATE LINKER:--build-id=none)
target_compile_options(nfcgate PRIVATE "-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=nfcd")
