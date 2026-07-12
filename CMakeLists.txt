cmake_minimum_required(VERSION 3.24)
project(HT_Game_Engine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define Base Paths
set(SDL_DIR "${CMAKE_SOURCE_DIR}/SDL")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/glad")
set(CORE_DIR "${CMAKE_SOURCE_DIR}/core")
set(GLM_DIR "${CMAKE_SOURCE_DIR}/glm")
set(UTILITY_DIR "${CMAKE_SOURCE_DIR}/utility")
set(SOURCES_DIR "${CMAKE_SOURCE_DIR}/src")


# Set Source Files

set(PROJECT_SOURCES	${SOURCES_DIR}/Main.cpp)

set(PROJECT_HEADERS
    ${CORE_DIR}/Camera.h ${CORE_DIR}/Engine.h ${CORE_DIR}/Log.h ${CORE_DIR}/Mesh.h 
    ${CORE_DIR}/Renderer.h ${CORE_DIR}/Shader.h ${CORE_DIR}/ECS.h 
	${CORE_DIR}/Components.h ${CORE_DIR}/Systems.h ${CORE_DIR}/Texture.h
    editor/EditorLayer.h
    ${UTILITY_DIR}/3DShapeBuilder.h ${UTILITY_DIR}/stb_image.h
)

set(PROJECT_UTILITY utility/3DShapeBuilder.h utility/MeshManager.h utility/json.hpp utility/SceneSerializer.h)

if(WIN32)
    add_library(SDL3_prebuilt SHARED IMPORTED)
    set_target_properties(SDL3_prebuilt PROPERTIES
        IMPORTED_LOCATION "${SDL_DIR}/lib/x64/SDL3.dll"
        IMPORTED_IMPLIB "${SDL_DIR}/lib/x64/SDL3.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL_DIR}/include"
    )
    
    add_library(SDL3_image_prebuilt SHARED IMPORTED)
    set_target_properties(SDL3_image_prebuilt PROPERTIES
        IMPORTED_LOCATION "${SDL_DIR}/lib/x64/SDL3_image.dll"
        IMPORTED_IMPLIB "${SDL_DIR}/lib/x64/SDL3_image.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL_DIR}/include"
    )
endif()

if(UNIX)
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Local/SDL3")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/Local/SDL3_image")
    find_package(SDL3 REQUIRED)
    find_package(SDL3_image REQUIRED)  
endif()

# Compile Local Dependencies
add_library(glad_lib STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad_lib PUBLIC ${GLAD_DIR}/include)

add_library(imgui_lib STATIC 
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends 
    ${CMAKE_SOURCE_DIR} 
)

# Define Executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_UTILITY})

# Linking Logic
if(WIN32)
    target_link_libraries(imgui_lib PUBLIC SDL3_prebuilt)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_prebuilt imgui_lib glad_lib)
else()
    target_link_libraries(imgui_lib PUBLIC SDL3::SDL3)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image imgui_lib glad_lib)
    
    # Correct Linux Include Paths
    target_include_directories(${PROJECT_NAME} PRIVATE 
        "$ENV{HOME}/Local/SDL3/include"
        "$ENV{HOME}/Local/SDL3_image/include"
    )
endif()

# Link & Include for Main Executable
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/editor
    ${GLM_DIR}
)

# 6. IDE/Post-Build Settings
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_UTILITY})

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DIR}/lib/x64/SDL3.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DIR}/lib/x64/SDL3_image.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
