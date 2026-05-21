#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!

#include "ECS.h"
#include "Systems.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "CubeBuilder.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <glm/glm.hpp>                  // Core vector/matrix math types
#include <glm/gtc/matrix_transform.hpp> // Matrix transformations (translate, rotate, scale, lookAt)
#include <glm/gtc/type_ptr.hpp>         // Allows us to pass GLM matrices directly to the GPU

// --- ImGui Includes ---
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

void SetupScene(Registry &registry, Mesh &cubeMesh);

// INPUT MAPPING STRUCTURE
// We store the state of keys here. This allows the logic to check "is W held?" 
// every single frame, regardless of when the key was originally pressed.
struct InputState {
    bool up = false, down = false, left = false, right = false;
    bool forward = false, backward = false;
};

SDL_Window* window = nullptr;
SDL_GLContext glContext = NULL;
Entity cube1;

int SDL_Initializaton(){
    // Initialize SDL3 Video Subsystem retrun true if successful, false if failed
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL3 Initialization Failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Request an OpenGL 4.6 Core Profile Contex
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create the Native Window Context
    window = SDL_CreateWindow(
        "HT Game Engine - Engine Context Verified",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL
    );

    if (!window) {
        std::cerr << "Failed to Create Window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Create the OpenGL Context bound to our Window
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to Create OpenGL Context: " << SDL_GetError() << std::endl;
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Initialize GLAD by feeding it SDL's function loader address
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to Initialize GLAD OpenGL Loader!" << std::endl;
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Disable VSync for uncapped frame rates (for testing)
    SDL_GL_SetSwapInterval(0); 

    return 0; // Success
}

int main(int argc, char* argv[]) {        
     SDL_Initializaton();
 
    // SUCCESS! Query the GPU to prove we are running hardware acceleration
    std::cout << "HT Game Engine Initialization Cleanly!" << std::endl;
    std::cout << "VENDOR:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "VERSION:  " << glGetString(GL_VERSION) << std::endl;
        
    // ==========================================================
    // DYNAMIC SHADER ASSET LOADING & COMPILATION
    // ==========================================================
    Shader myShader("shaders/opengl_vertex.glsl", "shaders/opengl_fragment.glsl"); 
    Camera myCamera;   

     // --- SETUP MESHES ---
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GetCubeData(vertices, indices);
    Mesh cubeMesh(vertices, indices);

    // --- SETUP ECS REGISTRY ---
    Registry registry;

    // Create entities and assign them components (e.g., Transform, Renderable)
    SetupScene(registry, cubeMesh); 

    
    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460");
  
    // Get the IO structure
    ImGuiIO& io = ImGui::GetIO();

    // Use the default font but set the size
    // The default ImGui font is internal, so we don't need a .ttf file path.
    // Changing '18.0f' to a higher number (e.g., 24.0f or 32.0f) will enlarge the text.
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.5f; // This makes the font itself 2x larger

    // This scales the entire UI globally (buttons, text, spacing)
    // ImGui::GetStyle().ScaleAllSizes(2.0f); // 2.0x zoom

    // --- SETUP INPUT & POSITION ---
    InputState input;   
    float moveSpeed = 0.01f;             

    // Enable depth testing for correct 3D rendering (closer objects should occlude farther ones)
    glEnable(GL_DEPTH_TEST); 

    // Accept fragment if it is closer to the camera than the former one
    glDepthFunc(GL_LESS); 
       
    bool isRunning = true;
    SDL_Event event;
    bool isDebug = false; // Toggle with TAB key 

    // Outside your while loop:
    Uint64 lastTime = SDL_GetTicks();
  
    // For controlling how often we print debug info to the console
    static float lastPrintTime = 0.0f; 
    static float fps = 0.0f;

    // The Master Game Loop
    while (isRunning) {
        // Calculate deltaTime for smooth movement regardless of frame rate
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert milliseconds to seconds
        lastTime = currentTime;
     

        // Process Native OS Input & Events
        while (SDL_PollEvent(&event)) {
            // Forward the event to ImGui
            // This allows ImGui to handle dragging, clicking, and resizing.
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false; // Player Clicked the 'X' button on the window
            }
            // Handle Keyboard: KEY_DOWN sets state to true, KEY_UP sets it to false.
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {                
                bool isDown = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.key){
                    case SDLK_W:      input.up = isDown; break;
                    case SDLK_S:      input.down = isDown; break;
                    case SDLK_A:      input.left = isDown; break;
                    case SDLK_D:      input.right = isDown; break;
                    case SDLK_Q:      input.forward = isDown; break; // Move closer
                    case SDLK_E:      input.backward = isDown; break; // Move further
                    case SDLK_ESCAPE: if (isDown) isRunning = false; break;
                    case SDLK_TAB: if (isDown) isDebug = !isDebug; break;
                }
                
            }
        }
        
        if(isDebug){
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode for debugging
        }
        else{
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Solid mode for normal rendering
        }

        // LOGIC UPDATE: Apply movement to the ECS entity        
        if (input.up)       registry.transforms[cube1].position.y += moveSpeed;
        if (input.down)     registry.transforms[cube1].position.y -= moveSpeed;
        if (input.left)     registry.transforms[cube1].position.x -= moveSpeed;
        if (input.right)    registry.transforms[cube1].position.x += moveSpeed;
        if (input.forward)  registry.transforms[cube1].position.z += moveSpeed;
        if (input.backward) registry.transforms[cube1].position.z -= moveSpeed;    
           
        // this line is calculating the exact number of seconds that have passed since your game engine booted up
        float time = (float)SDL_GetTicks() / 1000.0f;

        // Render Frames via OpenGL GPU Buffers; CLEAR THE SCREEN          
        glClearColor(0.1f, 0.14f, 0.18f, 1.0f);    

        // Clear the depth buffer as well for 3D rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         
     
        myShader.use(); // Activate the shader program (GPU will use this for rendering)
        myShader.setMat4("view", myCamera.GetViewMatrix()); // Send the View Matrix to the shader
        myShader.setMat4("projection", myCamera.GetProjectionMatrix(1024.0f / 768.0f)); // Send the Projection Matrix to the shader
        
        // Run Movement System to update positions based on velocity (if any)
        MovementSystem(registry, deltaTime);

        // Render all entities with a Transform and Renderable component
        RenderSystem(registry, myShader, time); 
       
        // --- ImGui Debug Overlay ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // DEBUG
        // We now pull the position directly from the ECS registry
        glm::vec3 pos = registry.transforms[cube1].position;
        glm::vec3 v1 = glm::vec3(-0.5f, -0.5f, 0.0f) + pos;
        glm::vec3 v2 = glm::vec3( 0.5f, -0.5f, 0.0f) + pos;
        glm::vec3 v3 = glm::vec3( 0.0f,  0.5f, 0.0f) + pos;

         

        if (currentTime - lastPrintTime >= 1000) { // Print every 1 second    
            fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;                        
            std::cout << "FPS: " << fps << std::endl;
            std::cout << "V1: " << v1.x << "," << v1.y << " | "
                << "V2: " << v2.x << "," << v2.y << " | "
                << "V3: " << v3.x << "," << v3.y << std::endl;
            lastPrintTime = currentTime;
        }

        // ImGui window for FPS and frame time debugging
        ImGui::Begin("Engine Stats");
        ImGui::Text("FPS: %.0f", fps);
        ImGui::Text("Frame Time: %.2f ms", deltaTime * 1000.0f);   
        ImGui::End(); // ImGui window for FPS end.

        // ImGui Window to display vertex positions and engine stats
        ImGui::Begin("Vertex Debugger");        
        ImGui::Text("V1: (%.2f, %.2f, %.2f)", v1.x, v1.y, v1.z);
        ImGui::Text("V2: (%.2f, %.2f, %.2f)", v2.x, v2.y, v2.z);
        ImGui::Text("V3: (%.2f, %.2f, %.2f)", v3.x, v3.y, v3.z);
        ImGui::Separator();
        ImGui::Text("Center: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::End();

       

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the back buffer to the front screen buffer to display what we drew
        SDL_GL_SwapWindow(window);

    }

    // Expliciet Clean Resource
    std::cout << "HT Game Engine shutting down cleanly..." << std::endl;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

// Pass by Reference to avoid copying the entire registry and mesh data structures
void SetupScene(Registry &registry, Mesh &cubeMesh){
    // Cube 1 (Controlled by input)
    cube1 = registry.CreateEntity();
    registry.transforms[cube1] = { glm::vec3(-1.0f, 0.0f, -2.0f) };
    registry.hasTransform[cube1] = true;
    registry.renderables[cube1] = { &cubeMesh };
    registry.hasRenderable[cube1] = true;

    // Cube 2 (Static)
    Entity cube2 = registry.CreateEntity();
    registry.transforms[cube2] = { glm::vec3(0.5f, 0.0f, 0.0f) };
    registry.hasTransform[cube2] = true;
    registry.renderables[cube2] = { &cubeMesh };
    registry.hasRenderable[cube2] = true;

}