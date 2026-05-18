
#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!
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

std::string loadShaderSourceFile(const char* filePath) {
    std::string shaderCode;
    std::ifstream shaderFile;

    // Configure file streams to explicitly throw exceptions if a read fails
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        shaderFile.open(filePath);
        std::stringstream shaderStream;

        // Stream the entire file buffer contents directly into our memory stream
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();

        // Convert the stream container into a usable C++ string
        shaderCode = shaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "CRITICAL ENGINE ERROR: Failed to read shader file at target path: " << filePath << std::endl;
    }

    return shaderCode;
}

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

    // 1. Load the GLSL raw text assets dynamically from your disk directory
    std::string vertexString = loadShaderSourceFile("shaders/opengl_vertex.glsl");
    std::string fragmentString = loadShaderSourceFile("shaders/opengl_fragment.glsl");

    // Extract the raw C-style string pointers required by the OpenGL driver
    const char* vertexShaderSource = vertexString.c_str();
    const char* fragmentShaderSource = fragmentString.c_str();

    // 2. Compile Vertex Shader on the GPU
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // 3. Compile Fragment Shader on the GPU
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 4. Link Shaders into a single executable GPU Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Clean up the individual intermediate shader objects once linked safely
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    bool isRunning = true;
    SDL_Event event;

    // Define our raw shape date in CPU memory (X, Y, Z)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // Bottom-Left point
         0.5f, -0.5f, 0.0f, // Bottom-Right point
         0.0f,  0.5f, 0.0f  // Top-Center Point
    };

    unsigned int VAO, VBO; // Vertex Buffer Object
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // --- Record Configuration Matrix ---
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Note: GL_STATIC_DRAW tells the GPU that we will set this data once and draw it 
    // many times, which optimizes it for extreme rendering speeds inside the card.

    // Define the Memory Layout Link (Vertex Attributes)
    // Tells the GPU: "Hey, take data location 0, look at 3 floats at a time, 
    // and the gap between each point is 3 times the size of a float."
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Clean Unbind Context
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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
    glm::vec3 objectPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Current translation
    float moveSpeed = 0.01f;                               // How fast we move

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
        glClear(GL_COLOR_BUFFER_BIT);

        // NEW: Tell the GPU to use our custom shader program
        glUseProgram(shaderProgram);

        // MODEL MATRIX: Apply movement (Translate) THEN rotation.
        // Order matters: We translate relative to (0,0,0), then rotate around that new spot.
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, objectPosition);

        // this line is calculating the exact number of seconds that have passed since your game engine booted up
        float time = (float)SDL_GetTicks() / 1000.0f;

        // Direction for the rotation matrix
        model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        // =================================================================
        // VIEW TRANSFORMATION (THE VIEW MATRIX)
        // =================================================================
        // Conceptually: This creates the illusion of a camera vantage point.
        // Mathematically: OpenGL has no camera; the viewer's eye is welded to (0,0,0).
        // Therefore, to make the player feel like they stepped BACK by +3.0 units, 
        // we must translate the entire WORLD's coordinates BACKWARD by -3.0 units 
        // down the Right-Handed negative Z-axis.
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

        // Right-Handed Cooridnate system move the objeectd away from the Screen or closer to the screen
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 
       
        // Projection Matrix: Perspective lens field of view (FOV)
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1024.0f / 768.0f, 0.1f, 100.0f);

        // Retrieve uniform handle locations from our compiled program
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc  = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc  = glGetUniformLocation(shaderProgram, "projection");
         
        // Upload C++ matrices directly into the GPU registers
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Bind our triangle configuration template 
        glBindVertexArray(VAO);

        // Issue the hardware drawing command (Draw 1 primitive triangle using 3 vertices)
        glDrawArrays(GL_TRIANGLES, 0, 3);

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

    // Deallocate local GPU buffer IDs cleanly before destroying context
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
