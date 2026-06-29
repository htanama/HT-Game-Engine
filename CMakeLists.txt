cmake_minimum_required(VERSION 3.24)
# PROJECT_NAME is used throughout to keep filenames/targets in sync
project(HT_Game_Engine LANGUAGES CXX C)

# Modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define Paths
set(SDL_DIR "${CMAKE_SOURCE_DIR}/SDL")
set(GLM_DIR "${CMAKE_SOURCE_DIR}/glm")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/glad")

# Project Files
set(PROJECT_SOURCES main.cpp)
set(PROJECT_HEADERS
    core/Camera.h core/Engine.h core/Log.h core/Mesh.h core/Renderer.h core/Shader.h
    core/ECS.h core/Components.h core/Systems.h
    editor/EditorLayer.h    
)
set(PROJECT_UTILITY utility/CubeBuilder.h utility/MeshManager.h utility/json.hpp utility/SceneSerializer.h)

# Import Precompiled SDL3
add_library(SDL3_prebuilt SHARED IMPORTED)
set_target_properties(SDL3_prebuilt PROPERTIES
    IMPORTED_LOCATION "${SDL_DIR}/lib/x64/SDL3.dll"
    IMPORTED_IMPLIB "${SDL_DIR}/lib/x64/SDL3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL_DIR}/include"
)

# Compile Local Dependencies
# GLAD
add_library(glad_lib STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad_lib PUBLIC ${GLAD_DIR}/include)

# ImGui
add_library(imgui_lib STATIC 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# LINKING FIX: This line solves your "SDL3/SDL.h not found" error
target_link_libraries(imgui_lib PUBLIC SDL3_prebuilt)
target_include_directories(imgui_lib PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# Define Executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_UTILITY})

# This tells CMake to group files in the IDE based on their path
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_UTILITY})

# OR, to be very specific about your Utility folder:
source_group("Utility" FILES ${PROJECT_UTILITY})

# Force Visual Studio to set your project as the default startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Automatically set the Visual Studio working directory to the executable's output folder
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# Link & Include
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SDL3_prebuilt
    imgui_lib
    glad_lib
)


target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/editor
    ${GLM_DIR}
)

# Post-Build: Copy DLL
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL_DIR}/lib/x64/SDL3.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

#  Shader Staging (Asset Pipeline)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Syncing shaders to build directory for local execution"
)