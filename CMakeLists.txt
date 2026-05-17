cmake_minimum_required(VERSION 3.20)
project(HT_Game_Engine LANGUAGES CXX C) # <-- MUST have 'CXX' and 'C' explicitly written here

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dynamically find all engine source files relatively
file(GLOB_RECURSE SOURCES "src/*.cpp")
set (GLAD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c")

add_executable(${PROJECT_NAME} ${SOURCES} ${GLAD_SOURCE})

# --- DETECT WINDOWS AND LINK RELATIVE SDL3 BUNDLE ---
if(WIN32)
    # ${CMAKE_CURRENT_SOURCE_DIR} points to your root directory 
    set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3-3.4.8/cmake")

    set(SDLCPU_TARGET "x64" CACHE STRING "Force x64 target to skip try_compile check")
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

# Tell the compiler where to find headers for BOTH your code and GLAD's glad.h
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    "${CMAKE_CURRENT_SOURCE_DIR}/glad/include"
    
)