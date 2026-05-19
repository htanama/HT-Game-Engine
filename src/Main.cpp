#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!
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


// INPUT MAPPING STRUCTURE
// We store the state of keys here. This allows the logic to check "is W held?" 
// every single frame, regardless of when the key was originally pressed.
struct InputState {
    bool up = false, down = false, left = false, right = false;
    bool forward = false, backward = false;
};

int main(int argc, char* argv[]) {
        
    // Initialize SDL3 Video Subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL3 Initialization Failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Request an OpenGL 4.6 Core Profile Contex
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create the Native Window Context
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

    // Create the OpenGL Context bound to our Window
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
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

    bool isRunning = true;
    SDL_Event event;

    // Define our raw shape date in CPU memory (X, Y, Z)
    float triangleVertices[] = {
        -0.5f, -0.5f, 0.0f, // Bottom-Left point
         0.5f, -0.5f, 0.0f, // Bottom-Right point
         0.0f,  0.5f, 0.0f  // Top-Center Point
    };


    // Square centered at 0,0
    float squareVertices[] = {
        -0.5f,  0.5f, 0.0f, // Top Left
        -0.5f, -0.5f, 0.0f, // Bottom Left
         0.5f, -0.5f, 0.0f, // Bottom Right

        -0.5f,  0.5f, 0.0f, // Top Left
         0.5f, -0.5f, 0.0f, // Bottom Right
         0.5f,  0.5f, 0.0f  // Top Right
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GetCubeData(vertices, indices);

    // Instantiate the mesh object
    // Mesh triangle(triangleVertices, sizeof(triangleVertices) / sizeof(float));
    
    //Mesh square(squareVertices, 18);

    // 36 vertices for the cube, each with 3 floats (x,y,z)
    Mesh cube(vertices, indices);

    
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
    glm::vec3 objectPosition = glm::vec3(-1.0f, 0.0f, 0.0f); // Current translation
    float moveSpeed = 0.01f;                               // How fast we move

    // Enable depth testing for correct 3D rendering (closer objects should occlude farther ones)
    glEnable(GL_DEPTH_TEST); 

    glDepthFunc(GL_LESS); // Accept fragment if it is closer to the camera than the former one
    
    // Wireframe mode for debugging
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
    
    // The Master Game Loop
    while (isRunning) {
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

                switch (event.key.key)
                {
                case SDLK_W:      input.up = isDown; break;
                case SDLK_S:      input.down = isDown; break;
                case SDLK_A:      input.left = isDown; break;
                case SDLK_D:      input.right = isDown; break;
                case SDLK_Q:      input.forward = isDown; break; // Move closer
                case SDLK_E:      input.backward = isDown; break; // Move further
                case SDLK_ESCAPE: if (isDown) isRunning = false; break;
                }
                
            }
        }

        // LOGIC UPDATE
        // We use the 'input' struct to update our objectPosition.
        // This is decoupled from the event loop, making it framerate-stable.
        if (input.up)       objectPosition.y += moveSpeed;
        if (input.down)     objectPosition.y -= moveSpeed;
        if (input.left)     objectPosition.x -= moveSpeed;
        if (input.right)    objectPosition.x += moveSpeed;
        if (input.forward)  objectPosition.z += moveSpeed;
        if (input.backward) objectPosition.z -= moveSpeed;

        // --- DEBUG: CALCULATE CURRENT VERTEX POSITIONS ---
        // Since we are applying translation to the whole model, we add the current
        // 'objectPosition' to our original raw coordinates.
        glm::vec3 v1 = glm::vec3(-0.5f, -0.5f, 0.0f) + objectPosition;
        glm::vec3 v2 = glm::vec3(0.5f, -0.5f, 0.0f) + objectPosition;
        glm::vec3 v3 = glm::vec3(0.0f, 0.5f, 0.0f) + objectPosition;

        // Print to console once per second (so it doesn't spam your terminal)
        static Uint64 lastTime = 0;
        if (SDL_GetTicks() - lastTime > 1000) {
            std::cout << "V1: " << v1.x << "," << v1.y << " | "
                << "V2: " << v2.x << "," << v2.y << " | "
                << "V3: " << v3.x << "," << v3.y << std::endl;
            lastTime = SDL_GetTicks();
        }
           
        // Render Frames via OpenGL GPU Buffers
        // Clear the screen with a beautiful, custom gray-blue background color
        glClearColor(0.1f, 0.14f, 0.18f, 1.0f);    

        // Clear the depth buffer as well for 3D rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        // PIPELINE EXECUTION
        myShader.use();

        // Draw Triangle on the LEFT (x = -0.5)
        // MODEL MATRIX: Apply movement (Translate) THEN rotation.
        // Order matters: We translate relative to (0,0,0), then rotate around that new spot.
        // glm::mat4 modelTriangle = glm::mat4(1.0f);
        // modelTriangle = glm::translate(modelTriangle, objectPosition);

        // this line is calculating the exact number of seconds that have passed since your game engine booted up
        float time = (float)SDL_GetTicks() / 1000.0f;

        // Start with an identity matrix (no transformation)
        glm::mat4 cubeModel = glm::mat4(1.0f); 

        cubeModel = glm::translate(cubeModel, objectPosition);

        // Direction for the rotation matrix
        cubeModel = glm::rotate(cubeModel, time * glm::radians(50.0f), glm::vec3(0.5f, -1.0f, 0.0f));

   
        // Draw Square on the RIGHT (x = 0.8)
        // glm::mat4 modelSquare = glm::translate(glm::mat4(1.0f), glm::vec3(0.8f, 0.0f, 0.0f));
        // myShader.setMat4("model", modelSquare);
        // square.draw();


        // =================================================================
        // VIEW TRANSFORMATION (THE VIEW MATRIX)
        // =================================================================
        // Conceptually: This creates the illusion of a camera vantage point.
        // Mathematically: OpenGL has no camera; the viewer's eye is welded to (0,0,0).
        // Therefore, to make the player feel like they stepped BACK by +3.0 units, 
        // we must translate the entire WORLD's coordinates BACKWARD by -3.0 units 
        // down the Right-Handed negative Z-axis.
        
        //glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        glm::mat4 view = myCamera.GetViewMatrix();

        // Right-Handed Cooridnate system move the objeectd away from the Screen or closer to the screen
        // view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 
       
        // Projection Matrix: Perspective lens field of view (FOV)
        //glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);
        glm::mat4 projection = myCamera.GetProjectionMatrix(1024.0f / 768.0f);

        myShader.setMat4("model", cubeModel);
        myShader.setMat4("view", view);
        myShader.setMat4("projection", projection);

        // --- RENDER MESH ---
        // Instead of binding VAO and drawing manually, use the object!
        cube.draw();

        // --- ImGui Debug Overlay ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Vertex Debugger");
        ImGui::Text("V1: (%.2f, %.2f, %.2f)", -0.5f + objectPosition.x, -0.5f + objectPosition.y, 0.0f + objectPosition.z);
        ImGui::Text("V2: (%.2f, %.2f, %.2f)", 0.5f + objectPosition.x, -0.5f + objectPosition.y, 0.0f + objectPosition.z);
        ImGui::Text("V3: (%.2f, %.2f, %.2f)", 0.0f + objectPosition.x, 0.5f + objectPosition.y, 0.0f + objectPosition.z);
        
        // Optional: Display the object's origin point for easier reference
        ImGui::Separator();
        ImGui::Text("Center: (%.2f, %.2f, %.2f)", objectPosition.x, objectPosition.y, objectPosition.z);
        
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
