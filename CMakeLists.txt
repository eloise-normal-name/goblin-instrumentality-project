cmake_minimum_required(VERSION 3.28)
project(goblin-stream CXX)

if(CMAKE_CONFIGURATION_TYPES)
    list(FIND CMAKE_CONFIGURATION_TYPES "RelWithDbgInfo" has_relwithdbginfo)
    if(has_relwithdbginfo EQUAL -1)
        list(APPEND CMAKE_CONFIGURATION_TYPES RelWithDbgInfo)
        set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
    endif()
endif()

set(CMAKE_CXX_FLAGS_RELWITHDBGINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDBGINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDBGINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)

# 1. Settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
# 2. Static Linking (/MT and /MTd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 3. Explicit Source Listing
set(SOURCES
    src/app.ixx
    src/main.cpp
    src/encoder/bitstream_file_writer.cpp
    src/encoder/frame_encoder.cpp
    src/encoder/nvenc_session.cpp
    src/graphics/command_allocators.cpp
    src/graphics/commands.cpp
    src/graphics/device.cpp
    src/graphics/frame_resources.cpp
    src/graphics/mesh.cpp
    src/graphics/pipeline.cpp
    src/graphics/resources.cpp
    src/graphics/swap_chain.cpp
)

# 4. Define Executable (WIN32 = subsystem:windows)
add_executable(goblin-stream WIN32 ${SOURCES})

# 5. Include Directories
target_include_directories(goblin-stream PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/include"
)

# 6. Compiler Flags (Warning Level 4)
target_compile_options(goblin-stream PRIVATE /W4 /EHs)

# 7. Definitions
target_compile_definitions(goblin-stream PRIVATE UNICODE _UNICODE)
target_compile_definitions(
    goblin-stream PRIVATE
    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:RelWithDbgInfo>>:ENABLE_FRAME_DEBUG_LOG>"
)

# 8. Output Directory Configuration (Match existing bin/ structure)
set_target_properties(goblin-stream PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDBGINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDbgInfo"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
)

# 9. Link Dependencies
target_link_libraries(goblin-stream PRIVATE d3d12 dxgi dxguid d3dcompiler)

