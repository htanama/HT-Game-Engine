cmake_minimum_required(VERSION 3.20)
project(HT_Game_Engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dynamically find all engine source files relatively
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

# --- DETECT WINDOWS AND LINK RELATIVE SDL3 BUNDLE ---
if(WIN32)
    # ${CMAKE_CURRENT_SOURCE_DIR} points to your root directory: D:/Windows-Dev-Projects/HT-Game-Engine
    set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3-3.4.8/cmake")
    find_package(SDL3 REQUIRED CONFIG)
    
    # Post-build: Copy the SDL3.dll alongside your final .exe file automatically
    # This prevents the annoying "SDL3.dll was not found" error when you hit run
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        VERBATIM
    )
else()
    # On Linux, look for the system-wide package instead
    find_package(SDL3 REQUIRED)
endif()

# Bind everything together
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
target_include_directories(${PROJECT_NAME} PRIVATE src)