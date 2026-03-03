cmake_minimum_required(VERSION 3.10)
project(Chess-Bot_Engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Hardware Optimization Toggle
option(USE_NATIVE_ARCH "Optimize for current CPU" ON)

# 2. Define the Executable
add_executable(Chess-Bot_Engine
    src/board.cpp
    src/engine.cpp
    src/evaluation.cpp
    src/main.cpp
    src/MoveGenerator.cpp
    src/notation_utils.cpp
    src/zobrist.cpp
    src/uci.cpp
)

target_include_directories(Chess-Bot_Engine PUBLIC include)

# 3. Apply optimization flags per compiler / platform
if(MSVC)
    # --- Release: maximum speed ---
    set(RELEASE_FLAGS /O2 /GL /Oi /Ot /Ob2 /DNDEBUG /Gy)
    if(USE_NATIVE_ARCH)
        list(APPEND RELEASE_FLAGS /arch:AVX2)
    endif()

    target_compile_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    )
    target_link_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )

    # --- RelWithDebInfo: optimized + good symbols for VS2022 CPU sampling (no /GL,/LTCG,/PROFILE) ---
    target_compile_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Ot /Ob1 /DNDEBUG /Zi /Zo>
    )
    target_link_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:RelWithDebInfo>:/DEBUG:FULL /OPT:REF /OPT:ICF /INCREMENTAL:NO>
    )

    if(USE_NATIVE_ARCH)
        target_compile_options(Chess-Bot_Engine PRIVATE
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
        )
    endif()

else()
    # --- GCC/Clang (Linux/macOS) ---
    # Release: maximum speed (good for Raspberry Pi too)
    set(RELEASE_FLAGS -O3 -DNDEBUG -flto)

    # CPU tuning:
    # - On ARM/AArch64 (e.g., Raspberry Pi 5 64-bit): -mcpu=native (best when building on the Pi)
    # - On x86_64: -march=native for local builds, else a portable baseline
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|arm64")
        if(USE_NATIVE_ARCH)
            list(APPEND RELEASE_FLAGS -mcpu=native)
        else()
            # Raspberry Pi 5 CPU (useful if cross-compiling)
            list(APPEND RELEASE_FLAGS -mcpu=cortex-a76)
        endif()
         # Fix AArch64 TLS relocation overflow with thread_local + LTO
        list(APPEND RELEASE_FLAGS -mtls-size=32)
    else()
        if(USE_NATIVE_ARCH)
            list(APPEND RELEASE_FLAGS -march=native)
        else()
            # Reasonably portable modern x86-64 baseline (GCC/Clang)
            list(APPEND RELEASE_FLAGS -march=x86-64-v3)
        endif()
    endif()

    target_compile_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:Release>:${RELEASE_FLAGS}>
    )
    target_link_options(Chess-Bot_Engine PRIVATE
        $<$<CONFIG:Release>:-flto>
    )
endif()

# Enable LTO (CMake's built-in way) for Release
set_target_properties(Chess-Bot_Engine PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

install(TARGETS Chess-Bot_Engine RUNTIME DESTINATION bin)