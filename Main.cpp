#include <glad/glad.h>

#include <SDL3/SDL.h>
#include "core/Engine.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "editor/EditorLayer.h"
#include "core/Camera.h"

bool Engine::isRunning = true;

int main(int argc, char* argv[]) {    
    
    SDL_Init(SDL_INIT_VIDEO);
    
    Renderer renderer;
    
    SDL_Window* window = SDL_CreateWindow("Engine", 
        renderer.GetWindowWidth(), 
        renderer.GetWindowHeight(), 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);  
   
    Shader myShader("shaders/triangle_vertex.glsl", "shaders/triangle_fragment.glsl");
    Shader gridShader("shaders/grid_vertex.glsl", "shaders/grid_fragment.glsl");

    std::vector<Vertex> triangleVertices = {
        //   X   ,   Y   ,  Z   ,  Normal, TexCoords,   Tangent     Bitangent,  m_BoneIDs, m_Weights
        { { -0.5f, -0.5f, 0.0f }, {0,0,0},  {0,0},      {0,0,0},    {0,0,0},    {0},        {0} },
        { {  0.5f, -0.5f, 0.0f }, {0,0,0},  {0,0},      {0,0,0},    {0,0,0},    {0},        {0} },
        { {  0.0f,  0.5f, 0.0f }, {0,0,0},  {0,0},      {0,0,0},    {0,0,0},    {0},        {0} }
    };

    std::vector<unsigned int> triangleIndices = {}; 
    std::vector<Texture> triangleTextures = {}; 

    Mesh myTriangle(triangleVertices, triangleIndices, triangleTextures);

    EditorLayer editor;
    editor.Init(window, context);
    Camera editorCamera;
    editorCamera.MovementSpeed = 1.0f;
    editorCamera.position = glm::vec3(0.0f, 5.0f, 10.0f);

    SDL_Event event;
    Uint64 lastTime = SDL_GetTicks();

    while (Engine::isRunning) {
        // Calculate deltaTime
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentTime - lastTime) / 1000.f; // covert from millisecond to second
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) Engine::SetIsRunning(false);

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Check if Right Mouse Button (SDL_BUTTON_RMASK) is pressed
                if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK) {
                    float xOffset = event.motion.xrel;
                    float yOffset = event.motion.yrel; // Inverted Y for standard FPS feel
                    editorCamera.RotateCamera(xOffset, yOffset); //
                }
            }

            // Handle Window Resize
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                renderer.currentWindowWidth = event.window.data1;
                renderer.currentWindowHeight = event.window.data2;

                // 1. Update OpenGL Viewport
                glViewport(0, 0, renderer.currentWindowWidth ,renderer.currentWindowHeight);

                // Update Renderer FBO
                renderer.WindowResize(renderer.currentWindowWidth, renderer.currentWindowHeight);
            }
        }

        // 3. WASD Movement (Polling for continuous input)
        const bool* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) editorCamera.ProcessKeyboard(FORWARD, deltaTime);
        if (state[SDL_SCANCODE_S]) editorCamera.ProcessKeyboard(BACKWARD, deltaTime);
        if (state[SDL_SCANCODE_A]) editorCamera.ProcessKeyboard(LEFT, deltaTime);
        if (state[SDL_SCANCODE_D]) editorCamera.ProcessKeyboard(RIGHT, deltaTime);

        // Render Game Scene
        renderer.DrawClearScreen(0.45f, 0.81f, 0.97f, 1.0f);           

        renderer.Draw(myTriangle, myShader);
        
        float aspect = (float)renderer.currentWindowWidth / renderer.currentWindowHeight;
        editor.Draw(editorCamera, gridShader, aspect);
        
        // Render UI
        editor.Begin();
        editor.Draw();
        editor.End();

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Logger::Log("OpenGL Error: " + std::to_string(err) + " detected in RenderLoop");
        }

        
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}