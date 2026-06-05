#include <SDL3/SDL.h>
#include "glad/glad.h"
#include "core/Renderer.h"
#include "editor/EditorLayer.h"

static const int WINDOW_WIDTH = 1920;
static const int WINDOW_HEIGHT = 1080;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Engine", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    Renderer renderer;
    renderer.Init();

    EditorLayer editor;
    editor.Init(window, context);

    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) isRunning = false;
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