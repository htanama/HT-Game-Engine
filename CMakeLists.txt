cmake_minimum_required(VERSION 3.20)
# PROJECT_NAME is used throughout to keep filenames/targets in sync
project(HT_Game_Engine LANGUAGES CXX C)

# Modern C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define ImGui Path (Assume you put imgui in a folder named 'imgui' in your root)
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

# Source Gathering
# Use GLOB_RECURSE to find all .cpp files automatically in the src/ folder.
# We also manually include glad.c because it is a C library, not a standard project file.
file(GLOB_RECURSE SOURCES "src/*.cpp")
set(IMGUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp 
    ${IMGUI_DIR}/imgui_draw.cpp 
    ${IMGUI_DIR}/imgui_tables.cpp 
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c" "src/Shader.h")

# Include Paths
# PRIVATE means these headers are only needed by this project (not by anything that uses this engine).
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    glad/include
    glm   
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends # Important: Allows you to include the backend headers
)

# --- WINDOWS SDL3 LINKING & ASSET STAGING ---
# Windows requires the .dll file to be in the same folder as the .exe.
if(WIN32)
    find_package(SDL3 REQUIRED CONFIG)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
    
    # POST_BUILD: After the executable is compiled, run this step.
    # copy_if_different: Only copies the .dll if it's new/missing (saves build time).
    # TARGET_FILE:SDL3::SDL3: Automatically finds where SDL3 was installed on your system.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

    # 3. Shader Staging (Asset Pipeline)
    # This keeps our "Source World" (root/shaders) synced with our "Build World" (binary/shaders).
    # This prevents 'file not found' errors when running the executable from the build folder.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        COMMENT "Syncing shaders to build directory for local execution"
    )

# --- LINUX / OTHER PLATFORMS ---
else()
    # On Linux, SDL3 is usually installed via package manager (apt/dnf), 
    # so we don't need to copy .dlls; the system handles the linking.
    find_package(SDL3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
    
    # Note: If you add more assets (textures/models) later, add them here 
    # to keep the Linux build consistent with the Windows staging process.
endif()