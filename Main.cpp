#include <SDL3/SDL.h>
#include "glad/glad.h"
#include "core/Engine.h"
#include "core/Renderer.h"
#include "editor/EditorLayer.h"

// static const int WINDOW_WIDTH = 1920;
// static const int WINDOW_HEIGHT = 1080;

Renderer renderer;
bool Engine::isRunning = true;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Engine", renderer.GetWindowWidth(), renderer.GetWindowHeight(), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    renderer.Init();

    EditorLayer editor;
    editor.Init(window, context);
   
    SDL_Event event;

    while (Engine::isRunning) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) Engine::SetIsRunning(false);

            // Handle Window Resize
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                int newWidth = event.window.data1;
                int newHeight = event.window.data2;

                // 1. Update OpenGL Viewport
                glViewport(0, 0, newWidth, newHeight);

                // Update Renderer FBO
                renderer.WindowResize(newWidth, newHeight);
            }
        }

        // Render Game Scene
        renderer.Draw();

        // Render UI
        editor.Begin();
        editor.Draw();
        editor.End();

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}