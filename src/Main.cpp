
#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!
#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // 1. Initialize SDL3 Video Subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL3 Initialization Failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 2. Request an OpenGL 4.6 Core Profile Context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // 3. Create the Native Window Context
    SDL_Window* window = SDL_CreateWindow(
        "HT Game Engine - Engine Context Verified",
        1024,
        768,
        SDL_WINDOW_OPENGL
    );

    if (!window) {
        std::cerr << "Failed to Create Window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // 4. Create the OpenGL Context bound to our Window
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to Create OpenGL Context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 5. Initialize GLAD by feeding it SDL's function loader address
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to Initialize GLAD OpenGL Loader!" << std::endl;
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Success! Query the GPU to prove we are running hardware acceleration
    std::cout << "HT Game Engine Initialized Cleanly!" << std::endl;
    std::cout << "VENDOR:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "VERSION:  " << glGetString(GL_VERSION) << std::endl;

    // Keep the engine sandbox alive for 4 seconds to view terminal metrics
    // SDL_Delay(4000);

    // --- NEW: CORE ENGINE LIFE WORKBENCH ---
    bool isRunning = true;
    SDL_Event event;

    // The Master Game Loop
    while (isRunning) {

        // Pillar 1: Process Native OS Input & Events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false; // Player clicked the 'X' button on the window
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                // If player presses the Escape key, flag the engine to shut down cleanly
                if (event.key.key == SDLK_ESCAPE) {
                    isRunning = false;
                }
            }
        }

        // TODO: Update Engine Systems (Physics, Positions, Frame Timers go here)

        // Render Frames via OpenGL GPU Buffers
        // Clear the screen with a beautiful, custom dark tactical background color
        glClearColor(0.1f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap the back buffer to the front screen buffer to display what we drew
        SDL_GL_SwapWindow(window);
    }

    // Explicit Clean Resource 
    std::cout << "HT Game Engine shutting down cleanly..." << std::endl;
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}