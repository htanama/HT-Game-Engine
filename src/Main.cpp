#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!

#include "ECS.h"
#include "Systems.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "CubeBuilder.h"
#include "DebugRenderer.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <memory>
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

// Global References
std::shared_ptr<Mesh> playerMesh, playerCameraMesh;
std::shared_ptr<Mesh> floorMesh;
std::shared_ptr<Mesh> testCubeMesh, testCubeMesh2; 
std::shared_ptr<Mesh> projectileMesh;
Entity playerEntity, playerCameraEntity;
std::unique_ptr<DebugRenderer> debugRenderer;

// INPUT MAPPING STRUCTURE
// We store the state of keys here. This allows the logic to check "is W held?" 
// every single frame, regardless of when the key was originally pressed.
struct InputState {
    bool forward = false, backward = false, left = false, right = false;    
};

SDL_Window* window = nullptr;
SDL_GLContext glContext = NULL;

// Rotate editorCamera with mouse movement when in debug mode
bool isRightMouseButtonDown = false;

bool isDebug = false; // Toggle with TAB key 
bool needsSnap = true; // Flag to control the one-time snap editorCamera

void CreateImGUI_Editor(){
    
}

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
    // window = SDL_CreateWindow(
    //     "HT Game Engine",
    //     WINDOW_WIDTH,
    //     WINDOW_HEIGHT,
    //     SDL_WINDOW_OPENGL //| SDL_WINDOW_FULLSCREEN
    // );

    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    window = SDL_CreateWindow("HT Game Engine", 
        WINDOW_WIDTH, WINDOW_HEIGHT, flags);

    if (!window) {
        std::cerr << "Failed to Create Window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    int displayWidth, displayHeight;
    SDL_GetWindowSize(window, &displayWidth, &displayHeight);

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
    
    // Hide the mouse cursor    
    if (!SDL_HideCursor()) {
        SDL_Log("Failed to hide cursor: %s", SDL_GetError());
    }
  
    SDL_SetWindowRelativeMouseMode(window, true);
    
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
    Camera playerCamera;   

    // This camera won't move, it's just for the editor UI to show a static view of the scene
    Camera editorCamera;   

    // --- SETUP MESHES ---
    // Generate Player (1m x 2m x 1m) 
    std::vector<Vertex> verticesPlayer;
    std::vector<unsigned int> indicesPlayer;
    GetPlayerCubeData(verticesPlayer, indicesPlayer); 

    // Generate Floor (10m x 0.1x 10m)
    std::vector<Vertex> verticesFloor;
    std::vector<unsigned int> indicesFloor;
    GetCustomCubeData(verticesFloor, indicesFloor, 10.0f, 0.1f, 10.0f); // Create a flat cube for the floor

    // Create Cube Meshes
    std::vector<Vertex> verticesCube;
    std::vector<unsigned int> indicesCube;    
    GetCustomCubeData(verticesCube, indicesCube, 1.0f, 1.0f, 1.0f); // Standard cube for testing
    testCubeMesh = std::make_shared<Mesh>(verticesCube, indicesCube); // Create a shared pointer to the cube mesh

    std::vector<Vertex> verticesCube2;
    std::vector<unsigned int> indicesCube2;    
    GetCustomCubeData(verticesCube2, indicesCube2, 1.0f, 1.0f, 1.0f); // Standard cube for testing
    testCubeMesh2 = std::make_shared<Mesh>(verticesCube2, indicesCube2); // Create a shared pointer to the cube mesh

    std::vector<Vertex> playerVerticesCamera;
    std::vector<unsigned int> playerIndicesCamera;
    GetCustomCubeData(playerVerticesCamera, playerIndicesCamera, 0.2f, 0.2f, 0.2f); // Create a small cube for the player camera (float width, float height, float depth)
    playerCameraMesh = std::make_shared<Mesh>(playerVerticesCamera, playerIndicesCamera); // Create a shared pointer to the player camera mesh
    
    // Create Projectile Mesh (0.2m x 0.2m x 0.2m)
    std::vector<Vertex> verticesProjectile; 
    std::vector<unsigned int> indicesProjectile;
    GetCustomCubeData(verticesProjectile, indicesProjectile, 0.2f, 0.2f, 0.2f); // Create a small cube for the projectile

    playerMesh = std::make_shared<Mesh>(verticesPlayer, indicesPlayer); // Create a shared pointer to the cube mesh
    floorMesh = std::make_shared<Mesh>(verticesFloor, indicesFloor);  // Reuse the cube mesh for the floor (we'll scale it later)
    projectileMesh = std::make_shared<Mesh>(verticesProjectile, indicesProjectile); // Create a shared pointer to the projectile mesh
    debugRenderer = std::make_unique<DebugRenderer>();

    // --- SETUP ECS REGISTRY ---
    Registry registry;
    SetupScene(registry, *playerMesh); // Create entities and assign them components (e.g., Transform, Renderable)

    // Create entities and assign them components (e.g., Transform, Renderable)
    //SetupScene(registry, cubeMesh); 
    
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
    float moveSpeed = 1.00f;             

    // Enable depth testing for correct 3D rendering (closer objects should occlude farther ones)
    glEnable(GL_DEPTH_TEST); 

    // Accept fragment if it is closer to the camera than the former one
    glDepthFunc(GL_LESS); 
       
    bool isRunning = true;
    SDL_Event event;   

    // Outside your while loop:
    Uint64 lastTime = SDL_GetTicks();
  
    // For controlling how often we print debug info to the console
    static float lastPrintTime = 0.0f; 
    static float fps = 0.0f;
    float mouseX, mouseY;  

    // The Master Game Loop
    while (isRunning) {
        // Calculate deltaTime for smooth movement regardless of frame rate
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert milliseconds to seconds
        lastTime = currentTime;     

        // Calculate fresh every frame so it works even if the window is resized
        int width, height;

        // Use this instead of SDL_GetWindowSize to handle high-DPI screens correctly
        SDL_GetWindowSizeInPixels(window, &width, &height);

        // Update the Viewport to match the actual pixel dimensions
        glViewport(0, 0, width, height);

        SDL_GetWindowSize(window, &width, &height);
        if (height == 0) height = 1; // Prevent division by zero            
        float currentAspectRatio = (float)width / (float)height;

        // Process Native OS Input & Events
        while (SDL_PollEvent(&event)) {                   
            // Forward the event to ImGui
            // This allows ImGui to handle dragging, clicking, and resizing.
            ImGui_ImplSDL3_ProcessEvent(&event);         

            // set the right mouse button state for camera rotation in debug mode
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT) {
                isRightMouseButtonDown = true;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_RIGHT) {
                isRightMouseButtonDown = false;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if(isDebug){
                    // Use the editor camera when in edit mode                    
                    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode for debugging
                    if (isRightMouseButtonDown) {
                        editorCamera.RotateCamera(event.motion.xrel * 0.1f, event.motion.yrel * 0.1f);
                    }
                }else{
                    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Solid mode for normal rendering
                    // Use the player camera when playing
                    // capture mouse movement for camera rotation                         
                    playerCamera.RotateCamera(event.motion.xrel * 0.1f, event.motion.yrel * 0.1f);
                }
            }                         
            
         

            if (event.type == SDL_EVENT_QUIT) {
                isRunning = false; // Player Clicked the 'X' button on the window
            }          
            // Handle Keyboard: KEY_DOWN sets state to true, KEY_UP sets it to false.
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {                
                bool isDown = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.key){
                    case SDLK_W:      input.forward = isDown; break;
                    case SDLK_S:      input.backward = isDown; break;
                    case SDLK_A:      input.left = isDown; break;
                    case SDLK_D:      input.right = isDown; break;                  
                    case SDLK_ESCAPE: if (isDown) isRunning = false; break;
                    case SDLK_TAB: if (isDown) isDebug = !isDebug; break;
                }
                
            }

            // Shoot a projectile when the left mouse button is clicked
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {                
                Entity projectile = GetProjectile(registry);

                // Ensure we have space for the new entity's components
                if(projectile < registry.transforms.size()){ 
                    
                    // Where projectile spawns: at the player camera position FPS Game Style
                    registry.transforms[projectile].position = playerCamera.position + (playerCamera.front * 0.2f); // Spawn a bit in front of the camera   
                    registry.hasTransform[projectile] = true;
                    
                    // color the projectile bright red so it's visible
                    registry.colors[projectile] = { glm::vec3(1.0f, 0.0f, 0.0f) };
                    registry.hasColor[projectile] = true;

                    // Give it velocity based on where the camera is looking
                    registry.velocities[projectile].value = playerCamera.front * 15.0f; // 15 units/sec
                    registry.hasVelocity[projectile] = true;
                    
                    // Give it rotation
                    registry.rotations[projectile] = { 0.0f, glm::vec3(2.0f, 1.0f, 0.0f), 0.5f };
                    registry.hasRotation[projectile] = true;
                    
                    // Add lifetime
                    registry.lifetimes[projectile] = { 5.0f };
                    registry.hasLifetime[projectile] = true;
                    
                    // Visuals
                    registry.renderables[projectile] = { projectileMesh.get() };
                    registry.hasRenderable[projectile] = true;
                }
            }
        }
        
        // Get the direction the camera is facing but ignore the Y axis. 
        glm::vec3 cameraForward = glm::normalize(glm::vec3(playerCamera.front.x, 0.0f, playerCamera.front.z));
        glm::vec3 cameraRight   = glm::normalize(glm::cross(cameraForward, playerCamera.up));

        if(isDebug){
            SDL_SetWindowRelativeMouseMode(window, false);
            SDL_ShowCursor();
            
            // Snap if we just entered debug mode to see the player
            if(needsSnap){
                // Calculate vector from camera to player
                glm::vec3 playerPos = registry.transforms[playerEntity].position;
                glm::vec3 direction = glm::normalize(playerPos - editorCamera.position);
                // Update the camera's orientation to look at the player
                // Note: Assuming your Camera class has a way to set its view direction.
                // If your camera uses yaw/pitch, you'd calculate them from the direction vector:
                editorCamera.SetDirection(direction);
                needsSnap = false; // Snap done. 
            }

            float cameraSpeed = moveSpeed * 5.0f; // Move faster in debug mode            
            // In debug mode, we can move the camera freely like a spectator
            if (input.forward)  editorCamera.position += editorCamera.front * cameraSpeed * deltaTime;
            if (input.backward) editorCamera.position -= editorCamera.front * cameraSpeed * deltaTime;
            if (input.left)     editorCamera.position -= editorCamera.right * cameraSpeed * deltaTime;
            if (input.right)    editorCamera.position += editorCamera.right * cameraSpeed * deltaTime;
             
            registry.hasRenderable[playerCameraEntity] = true;            
            registry.renderables[playerCameraEntity].mesh = playerCameraMesh.get();

            registry.hasRenderable[playerEntity] = true;     
            registry.renderables[playerEntity].mesh = playerMesh.get();
        }
        else{ // Play Mode / Not On Debug Mode  
            SDL_SetWindowRelativeMouseMode(window, true);
            SDL_HideCursor();
            registry.hasRenderable[playerCameraEntity] = false; // Hide the camera cube in play mode
            registry.hasRenderable[playerEntity] = false; // hide mesh to fix rendering overlap with camera view


            // Calculate movement based on camera vectors
            glm::vec3 moveDir(0.0f);
            if (input.forward)    moveDir += cameraForward;
            if (input.backward)  moveDir -= cameraForward;
            if (input.left)  moveDir -= cameraRight;
            if (input.right) moveDir += cameraRight;

            // 3. Apply movement to the player position
            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir); // Prevent "diagonal speed boost"
                registry.transforms[playerEntity].position += moveDir * moveSpeed * deltaTime;
                registry.transforms[playerCameraEntity].position = playerCamera.position;
            }

             // Move Player Old School Style: Directly manipulate the position based on input
            // glm::vec3 inputVelocity(0.0f);
            // if (input.up) inputVelocity.z -= 1.0f;
            // if (input.down) inputVelocity.z += 1.0f;
            // if (input.left) inputVelocity.x -= 1.0f;
            // if (input.right) inputVelocity.x += 1.0f;
            // registry.transforms[playerEntity].position += inputVelocity * moveSpeed * deltaTime;

        }
        
        // this line is calculating the exact number of seconds that have passed since your game engine booted up
        float time = (float)SDL_GetTicks() / 1000.0f;

        // Render Frames via OpenGL GPU Buffers; CLEAR THE SCREEN   
        // A standard sky blue color       
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 

        // Clear the depth buffer as well for 3D rendering
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                                                

        // Pick which camera to use
        Camera& activeCamera = isDebug ? editorCamera : playerCamera;
        
        if (isDebug){
           // Draw X Axis (Red)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(50,0,0));
           debugRenderer->Render(activeCamera.GetViewMatrix(), activeCamera.GetProjectionMatrix(currentAspectRatio), glm::vec3(1, 0, 0));

           // Draw Y Axis (Green)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,50,0));
           debugRenderer->Render(activeCamera.GetViewMatrix(), activeCamera.GetProjectionMatrix(currentAspectRatio), glm::vec3(0, 1, 0));

           // Draw Z Axis (Blue)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,0,50));
           debugRenderer->Render(activeCamera.GetViewMatrix(), activeCamera.GetProjectionMatrix(currentAspectRatio), glm::vec3(0, 0, 1));
        }

        // Retrieve the position vector from the Transform component associated with the player entity
        glm::vec3 playerPos = registry.transforms[playerEntity].position;

        // Update the camera's position to be above the player's current position 
        // by setting it equal to the player's position plus a vertical offset (0.0f, 1.0f, 0.0f)
        if(!isDebug){            
            playerCamera.position = playerPos + glm::vec3(0.0f, 0.5f, 0.0f);                
        }
        // Call the use function to activate the shader program for rendering graphics
        myShader.use();
        
        // Set a uniform variable in the shader program with the name "viewPos" and pass the current camera position
        myShader.setVec3("viewPos", activeCamera.position);        

        // Set a uniform variable in the shader program with the name "view" and pass the view matrix of the camera
        myShader.setMat4("view", activeCamera.GetViewMatrix());

        // Set a uniform variable in the shader program with the name "projection" and pass the projection matrix of the camera, adjusting for window aspect ratio
        myShader.setMat4("projection", activeCamera.GetProjectionMatrix(currentAspectRatio));
        
        myShader.setVec3("objectColor", glm::vec3(1.0f)); // White light

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
        glm::vec3 pos = registry.transforms[playerEntity].position;
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

        if (isDebug)
        {
            // Define the right-hand panel area
            // float panelX = 1920.0f - 400.0f;            
            float panelWidth = 400.0f;

            ImGuiIO &io = ImGui::GetIO();
            float panelX = io.DisplaySize.x - panelWidth;

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

            // --- 1. Engine Stats (Top Right) ---
            ImGui::SetNextWindowPos(ImVec2(panelX, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(panelWidth, 150), ImGuiCond_Always);
            ImGui::Begin("Engine Stats", nullptr, windowFlags);
            ImGui::Text("FPS: %.0f", fps);
            ImGui::Text("Frame Time: %.2f ms", deltaTime * 1000.0f);
            ImGui::End();

            // --- 2. Vertex Debugger (Below Stats) ---
            float remainingHeight = io.DisplaySize.y - 150.0f; // Subtract top panel height
            ImGui::SetNextWindowPos(ImVec2(panelX, 150), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(panelWidth, 200), ImGuiCond_Always);
            // FIXED: The window flags and nullptr must be outside the string!
            ImGui::Begin("Vertex Player Debugger", nullptr, ImGuiWindowFlags_None); 
            ImGui::Text("V1: (%.2f, %.2f, %.2f)", v1.x, v1.y, v1.z);
            ImGui::Text("V2: (%.2f, %.2f, %.2f)", v2.x, v2.y, v2.z);
            ImGui::Text("V3: (%.2f, %.2f, %.2f)", v3.x, v3.y, v3.z);
            ImGui::Separator();
            ImGui::Text("Center: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
            ImGui::End();

            // --- 3. Inspector (Below Vertex Debugger) ---
            ImGui::SetNextWindowPos(ImVec2(panelX, 350), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(panelWidth, 200), ImGuiCond_Always);
            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_None);
            
            if (ImGui::CollapsingHeader("Player Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& t = registry.transforms[playerEntity];
                ImGui::DragFloat3("Position", &t.position.x, 0.1f);
                ImGui::DragFloat3("Scale",    &t.scale.x,    0.05f);
                if (ImGui::Button("Reset Player Scale")) {
                    t.scale = glm::vec3(1.0f);
                }
            }
            if (ImGui::CollapsingHeader("Camera Mesh")) {
                auto& c = registry.transforms[playerCameraEntity];
                ImGui::DragFloat3("Cam Position", &c.position.x, 0.1f);                
            }
            ImGui::End();

            
        }

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

    //Create Player (Cube)
    playerEntity = registry.CreateEntity();
    
    // Player at 0.0 Y puts the bottom of the player exactly on the floor
    registry.transforms[playerEntity].position = glm::vec3(0.0f, 0.5f, 0.0f);        
    registry.hasTransform[playerEntity] = true;
    registry.renderables[playerEntity] = { playerMesh.get() };
    registry.hasRenderable[playerEntity] = true;

    // Create player camera entity (small cube that shows where the camera is in the world, useful for debugging)
    playerCameraEntity = registry.CreateEntity();    
    registry.transforms[playerCameraEntity].position = glm::vec3(0.0f, 0.0f, 0.0f); // Start above the player
    registry.hasTransform[playerCameraEntity] = true;
    registry.renderables[playerCameraEntity] = { playerCameraMesh.get() };
    registry.hasRenderable[playerCameraEntity] = false; // Start with the camera cube invisible (we'll toggle it in debug mode)
    registry.colors[playerCameraEntity] = { glm::vec3(0.0f, 1.0f, 0.0f) }; // Green color for the camera cube
    registry.hasColor[playerCameraEntity] = true;     


    // Create Floor (Wide, thin cube)
    Entity floorEntity = registry.CreateEntity();
    registry.transforms[floorEntity] = { glm::vec3(0.0f, 0.0f, 0.0f) }; // Positioned below

    // Note: You can apply a scaling matrix in your RenderSystem based on 
    // a potential 'Scale' component if you want to make it look flat!
    registry.hasTransform[floorEntity] = true;
    registry.renderables[floorEntity] = { floorMesh.get() };
    registry.hasRenderable[floorEntity] = true;
    registry.colors[floorEntity] = { glm::vec3(0.1f, 0.3f, 0.1f) };
    registry.hasColor[floorEntity] = true;
    

    // testCube 
    Entity testCube = registry.CreateEntity();
    registry.transforms[testCube] = { glm::vec3(-1.0f, 1.0f, -10.0f) };
    registry.hasTransform[testCube] = true;
    registry.colors[testCube] = { glm::vec3(0.0f, 0.0f, 1.0f) }; // Red color for this cube
    registry.hasColor[testCube] = true;
    registry.renderables[testCube] = { testCubeMesh.get() };    
    registry.hasRenderable[testCube] = true;
    registry.rotations[testCube] = { 0.0f, glm::vec3(-1.0f, 0.5f, 0.0f), 0.1f }; // Rotate around Y-axis at 20 degrees per second
    registry.hasRotation[testCube] = true;


    Entity testCube2 = registry.CreateEntity();
    registry.transforms[testCube2] = { glm::vec3(5.0f, 1.0f, 10.0f) };
    registry.hasTransform[testCube2] = true;
    registry.colors[testCube2] = { glm::vec3(1.0f, 0.0f, 0.0f) }; // Red color for this cube
    registry.hasColor[testCube2] = true;
    registry.renderables[testCube2] = { testCubeMesh.get() };    
    registry.hasRenderable[testCube2] = true;
    registry.rotations[testCube2] = { 0.0f, glm::vec3(0.0f, 0.5f, 0.0f), 0.1f }; // Rotate around Y-axis at 20 degrees per second
    registry.hasRotation[testCube2] = true;


    // Projectile will be created dynamically when the player clicks, so we don't set it up here in the scene.

    /*/ Cube 2 (Static)
    Entity cube2 = registry.CreateEntity();
    registry.transforms[cube2] = { glm::vec3(0.5f, 0.0f, 0.0f) };
    registry.hasTransform[cube2] = true;
    registry.renderables[cube2] = { &cubeMesh };
    registry.hasRenderable[cube2] = true;
    */
}