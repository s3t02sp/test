cmake_minimum_required(VERSION 3.10)
project(HitLogImGui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ImGui sources (you need to have ImGui in your project)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

if(NOT EXISTS ${IMGUI_DIR})
    message(STATUS "ImGui not found. Downloading...")
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.1
    )
    FetchContent_MakeAvailable(imgui)
    set(IMGUI_DIR ${imgui_SOURCE_DIR})
endif()

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include directories
include_directories(
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_INCLUDE_DIR}
)

# Executable
add_executable(hit_log
    hit_log.cpp
    ${IMGUI_SOURCES}
)

# Link libraries
target_link_libraries(hit_log
    OpenGL::GL
    glfw
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(hit_log dl)
endif()
