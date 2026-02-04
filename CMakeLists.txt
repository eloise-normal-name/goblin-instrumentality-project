cmake_minimum_required(VERSION 3.28)
project(goblin-stream CXX)

# 1. Settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Static Linking (/MT and /MTd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 3. Explicit Source Listing
set(SOURCES
    src/main.cpp
    src/encoder/nvenc_config.cpp
    src/encoder/nvenc_d3d12.cpp
    src/encoder/nvenc_session.cpp
    src/graphics/d3d12_commands.cpp
    src/graphics/d3d12_device.cpp
    src/graphics/d3d12_resources.cpp
    src/pipeline/frame_coordinator.cpp
)

# 4. Define Executable (WIN32 = subsystem:windows)
add_executable(goblin-stream WIN32 ${SOURCES})

# 5. Include Directories
target_include_directories(goblin-stream PRIVATE "${CMAKE_SOURCE_DIR}/include")

# 6. Compiler Flags (Warning Level 4)
target_compile_options(goblin-stream PRIVATE /W4 /EHs)

# 7. Definitions
target_compile_definitions(goblin-stream PRIVATE UNICODE _UNICODE)

# 8. Output Directory Configuration (Match existing bin/ structure)
set_target_properties(goblin-stream PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
)

# 9. Link Dependencies
target_link_libraries(goblin-stream PRIVATE d3d12 dxgi dxguid)
