cmake_minimum_required(VERSION 3.15)
project(deltagen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For MSVC / clang-cl set static runtime
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(deltagen main.cpp)

#target_include_directories(deltagen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    # Windows Cryptography & Core System APIs
    target_link_libraries(deltagen PRIVATE Crypt32 Advapi32 User32)
endif()

# ==============================================================================
# Compiler & Toolchain Specific Optimizations (Static Linking)
# ==============================================================================

if(MINGW)
    # MinGW-w64 (GCC / Clang) Static Linking Flags
    target_compile_options(deltagen PRIVATE -O3)
    target_link_options(deltagen PRIVATE 
        -static 
        -static-libgcc 
        -static-libstdc++ 
        -Wl,--gc-sections
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND MSVC)
    # clang-cl (LLVM with MSVC ABI)
    target_compile_options(deltagen PRIVATE /O2 -flto)
    target_link_options(deltagen PRIVATE -flto /MANIFEST:NO)
elseif(MSVC)
    # Native MSVC cl.exe
    target_compile_options(deltagen PRIVATE /O2 /GL)
    target_link_options(deltagen PRIVATE /LTCG /INCREMENTAL:NO /MANIFEST:NO)
endif()