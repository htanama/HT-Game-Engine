#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!

#include "ECS.h"
#include "Systems.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "CubeBuilder.h"
#include "DebugRenderer.h"
#include "MeshManager.h"

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

enum class EditorState { Editor, Playing };
static EditorState gameState = EditorState::Editor;

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

std::vector<float> gridVertices;

// Global References
std::shared_ptr<Mesh> playerMesh = nullptr;
std::shared_ptr<Mesh> projectileMesh = nullptr;
std::shared_ptr<Mesh> playerCameraMesh = nullptr;
std::shared_ptr<Mesh> testCubeMesh = nullptr;
std::shared_ptr<Mesh> testCubeMesh2 = nullptr;
std::shared_ptr<Mesh> floorMesh = nullptr;

Entity playerEntity, playerCameraEntity;
std::unique_ptr<DebugRenderer> debugRenderer;
std::vector<std::string> logMessages;

Camera editorCamera;  

// INPUT MAPPING STRUCTURE
// We store the state of keys here. This allows the logic to check "is W held?" 
// every single frame, regardless of when the key was originally pressed.
struct InputState {
    bool forward = false, backward = false, left = false, right = false, lshift = false;    
};

SDL_Window* window = nullptr;
SDL_GLContext glContext = NULL;

static Entity selectedEntity = -1;
static bool autoScroll = true;
static bool isRenaming = false;
static char nameBuffer[64] = "";

// Rotate editorCamera with mouse movement when in debug mode
bool isRightMouseButtonDown = false;
bool isDebug = false; // Toggle with TAB key 
bool needsSnap = true; // Flag to control the one-time snap editorCamera
bool isRunning = true; // game is running or editor is running
bool requestCameraReset = false;

void RunGamePlay();

void Log(const std::string& message) {
    std::cout << message << std::endl;
    logMessages.push_back(message);
    
    // Keep only the last 100 messages to save memory
    if (logMessages.size() > 100) {
        logMessages.erase(logMessages.begin());
    }
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
  
    // SDL_SetWindowRelativeMouseMode(window, true);

    return 0; // Success
}

// this is old BuildMesh
std::shared_ptr<Mesh> BuildMesh(float w, float h, float d) {
    std::vector<Vertex> v;
    std::vector<unsigned int> i;
    GetCustomCubeData(v, i, w, h, d);
    return std::make_shared<Mesh>(v, i);
}
// This is old ways to create meshes
void InitializeMeshes() {
    playerMesh     = BuildMesh(0.16f, 1.0f, 0.16f); // Use your custom dimensions
    floorMesh      = BuildMesh(10.0f, 0.1f, 10.0f);
    projectileMesh = BuildMesh(0.2f, 0.2f, 0.2f);
    testCubeMesh   = BuildMesh(1.0f, 1.0f, 1.0f);
    testCubeMesh2  = BuildMesh(0.5f, 0.5f, 0.5f);  
}

int SetupGrid(unsigned int& vao, unsigned int& vbo) {
    std::vector<float> vertices;
    float size = 10.0f;
    for (float i = -size; i <= size; i++) {
        vertices.insert(vertices.end(), {-size, 0.0f, i, size, 0.0f, i});
        vertices.insert(vertices.end(), {i, 0.0f, -size, i, 0.0f, size});
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return (int)vertices.size() / 3; // Return the count directly
}

void DrawEditorUI(Registry& registry, Entity& selectedEntity, EditorState& gameState, float deltaTime, float fps) {
    ImGuiIO& io = ImGui::GetIO();
    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 2.0f;

    float leftPanelWidth = 300.0f;
    float rightPanelWidth = 400.0f;
    float bottomPanelHeight = 200.0f;
    float menuBarHeight = ImGui::GetFrameHeight();
    
    ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, screenH - menuBarHeight - bottomPanelHeight), ImGuiCond_Always);
    
    // Top Menu Bar 
    if (ImGui::BeginMainMenuBar()) {       
        float buttonWidthOrigin = 120.0f;
        float buttonWidthPlay = 80.0f;
        float spacing = 20.0f;
        float totalGroupWidth = buttonWidthOrigin + spacing + buttonWidthPlay;

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalGroupWidth) * 0.5f);

        if (ImGui::Button(" Origin ", ImVec2(buttonWidthOrigin,0))) {
            requestCameraReset = true;            
        }
        
        ImGui::SameLine(0, spacing); // Keep them on the same line
        // State-based button
        if (gameState == EditorState::Editor) {
            if (ImGui::Button(" Play ", ImVec2(buttonWidthPlay, 0))){ 
                gameState = EditorState::Playing; 
                needsSnap = false;
            }
        } else {
            // Optional: Make "Stop" red to signify an active game
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button(" Stop ", ImVec2(buttonWidthPlay, 0))){ 
                gameState = EditorState::Editor; 
                needsSnap = true;
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndMainMenuBar();
    }   
    ImGui::Separator();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    // Hierarchy 
    ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, screenH - 20 - bottomPanelHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Scene Hierarchy", nullptr, flags)) {
             // Add Button
        if (ImGui::Button("Add New Entity", ImVec2(-1, 0))) {
            Entity newEnt = registry.CreateEntity();
        
            // 1. Create and get the mesh from your library
            std::shared_ptr<Mesh> cubeMesh = CreateNewCubeMesh();
            
            // 2. Register components
            registry.AddTransform(newEnt, { glm::vec3(0.0f), glm::vec3(1.0f) });
            
            // Pass the shared_ptr from the library
            registry.renderables[newEnt].mesh = cubeMesh; 
            registry.hasRenderable[newEnt] = true;
            
            registry.AddColor(newEnt, { glm::vec3(1.0f, 1.0f, 1.0f) });
            registry.names[newEnt] = { "New Cube" };
            registry.hasName[newEnt] = true;

            registry.colors[newEnt].color = glm::vec3(1.0f, 1.0f, 1.0f); // Default white color
            registry.hasColor[newEnt] = true;
            Log("Added New Entity and total number of entities: " + std::to_string(registry.GetEntityCount()));
        }         
        //ImGui::Separator();
        
        // to check the correct entity to rename
        static Entity renamingEntity = (Entity)-1;        
        static char nameBuffer[64] = "";

        // to check the correct entity to delete
        static Entity entityToDelete = (Entity)-1;

        for (size_t i = 0; i < registry.hasTransform.size(); ++i) {
            if (!registry.hasTransform[i]) continue;

            ImGui::PushID((int)i);

            // Retrieve name
            std::string entityName = registry.hasName[i] ? registry.names[i].name : "Object " + std::to_string(i);

            // Logic: If we are currently renaming THIS specific entity
            if (renamingEntity == (Entity)i) {
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##rename", nameBuffer, sizeof(nameBuffer), 
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                    
                    registry.names[i].name = std::string(nameBuffer);
                    renamingEntity = (Entity)-1; // Exit rename mode
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) renamingEntity = (Entity)-1;
            } 
            else {
                // Normal display mode
                if (ImGui::Selectable(entityName.c_str(), selectedEntity == (Entity)i)) {
                    selectedEntity = (Entity)i;
                }

                // Trigger rename on double-click OR right-click menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    renamingEntity = (Entity)i;
                    strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
                }

                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename")) {
                        renamingEntity = (Entity)i;
                        strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
                    }
                    if (ImGui::MenuItem("Delete")) {
                        entityToDelete = (Entity)i; // Set delete target                        
                    }
                    ImGui::EndPopup();
                }
            }

            ImGui::PopID();
        }

        if (entityToDelete != (Entity)-1) {            
            RequestDeleteEntity(registry, entityToDelete);
            CleanupUnusedMeshes();
            
            // Reset selection if we deleted the currently selected one
            if (selectedEntity == entityToDelete) {
                selectedEntity = (Entity)-1;
            }
            registry.SubtractEntityCount();            
            Log("Entity ID " + std::to_string(entityToDelete) + " components cleared.");
            Log("Total entities: " + std::to_string(registry.GetEntityCount()));
            entityToDelete = (Entity)-1; // Reset target
        }

    }
    ImGui::End();


    // INSPECTOR      
    ImGui::SetNextWindowPos(ImVec2(screenW - rightPanelWidth, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, screenH), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, flags);


    if (selectedEntity != -1 && selectedEntity < registry.transforms.size() && registry.hasTransform[selectedEntity]) {
        auto& t = registry.transforms[selectedEntity];      
        // --- POSITION ---
        ImGui::Text("Position");
        ImGui::Columns(3, nullptr, false); 
        ImGui::Text("X"); ImGui::NextColumn();
        ImGui::Text("Y"); ImGui::NextColumn();
        ImGui::Text("Z"); ImGui::NextColumn();
        ImGui::Columns(1); // Close columns so the sliders aren't forced into them
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragFloat3("##Position", &t.position.x, 0.1f);

        // --- ROTATION ---
        ImGui::Text("Rotation");
        ImGui::Columns(3, nullptr, false);
        ImGui::Text("X"); ImGui::NextColumn();
        ImGui::Text("Y"); ImGui::NextColumn();
        ImGui::Text("Z"); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragFloat3("##Rotation", &t.rotation.x, 1.0f);

        // --- SCALE ---
        ImGui::Text("Scale");
        ImGui::Columns(3, nullptr, false);
        ImGui::Text("X"); ImGui::NextColumn();
        ImGui::Text("Y"); ImGui::NextColumn();
        ImGui::Text("Z"); ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragFloat3("##Scale", &t.scale.x, 0.05f);
    }

    if (selectedEntity != -1 && selectedEntity < registry.colors.size() && registry.hasColor[selectedEntity]) {
        auto& c = registry.colors[selectedEntity];
        
        // ColorEdit3 takes a float array (a pointer to the first of 3 floats)
        if (ImGui::ColorEdit3("Object Color", &c.color.x)) {
            // This block runs automatically whenever the user changes the color.
            // If your renderer is already using registry.colors[e].color 
            // in its main loop, this will work instantly!
        }
    }

    ImGui::Separator();
    ImGui::Separator();
    // Add this block to show camera/vertex data
    if (ImGui::CollapsingHeader("Camera Info")) {
        // Displaying the vector components
        ImGui::Text("Position X: %.2f", editorCamera.position.x);
        ImGui::Text("Position Y: %.2f", editorCamera.position.y);
        ImGui::Text("Position Z: %.2f", editorCamera.position.z);                  
                     
        ImGui::Text("Pitch: %.2f", editorCamera.pitch);
        ImGui::Text("Yaw: %.2f", editorCamera.yaw);
    }


    ImGui::End(); 
        
    // Output Console 
    float consoleWidth = screenW * 0.7f; // Example: Make console 80% of screen width
    float centerX = (screenW - consoleWidth) * 0.5f;

    // Flags: ImGuiCond_Always or ImGuiCond_FirstUseEver
    ImGui::SetNextWindowPos(ImVec2(centerX, screenH - bottomPanelHeight), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(consoleWidth, bottomPanelHeight), ImGuiCond_FirstUseEver);    

    ImGui::Begin("Output", nullptr);
    float buttonWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);

    if (ImGui::Button("Clear")) {
        logMessages.clear();
    }

    ImGui::Separator();
    ImGui::SameLine(); 
    ImGui::Separator();
    
    // Send all logMessages to the ImGUI Output Console
    for (const auto& msg : logMessages) {
        ImGui::TextUnformatted(msg.c_str());
    }

    // Check if we want to scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::End(); 
  
}// Draw ImGUI END

void GetCurrentFrame(const Uint64 &currentTime, Uint64 &lastTime, const float &deltaTime){
    ImGui::Begin("Performance Monitor");
    
    // Just for testing, see if this value changes
    ImGui::Text("Raw DeltaTime: %f", deltaTime); 
    
    float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    ImGui::Text("Calculated FPS: %.1f", fps);

    ImGui::End();
}



int main(int argc, char* argv[]) {        
    SDL_Initializaton();
    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();    
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460");      
    

    debugRenderer = std::make_unique<DebugRenderer>();
    Uint64 lastTime = SDL_GetTicks();
    Shader myShader("shaders/opengl_vertex.glsl", "shaders/opengl_fragment.glsl");     
    Shader debug_lineShader("shaders/debug_line_vertex.glsl", "shaders/debug_line_fragment.glsl");
    SDL_Event event; 
    Registry registry;
    InputState input;     
    float moveSpeed = 1.00f;   
    editorCamera.position = glm::vec3(0.0f, 5.0f, 15.0f);
    
     // SUCCESS! Query the GPU to prove we are running hardware acceleration
    Log("HT Game Engine Initialization Cleanly!");
    Log("VENDOR:   " + std::string((const char*)glGetString(GL_VENDOR)));
    Log("RENDERER: " + std::string((const char*)glGetString(GL_RENDERER)));
    Log("VERSION:  " + std::string((const char*)glGetString(GL_VERSION)));
 

    unsigned int gridVAO, gridVBO;
    int count = SetupGrid(gridVAO, gridVBO);

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
        float mouseDeltaX, mouseDeltaY;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);    

            // Handle Mouse Button Toggles
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_RIGHT)
                isRightMouseButtonDown = true;
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_RIGHT)
                isRightMouseButtonDown = false;

            // Capture movement only when right-click is held
            if (event.type == SDL_EVENT_MOUSE_MOTION && isRightMouseButtonDown) {
                mouseDeltaX += event.motion.xrel;
                mouseDeltaY += event.motion.yrel;
            }      

            if (event.type == SDL_EVENT_QUIT) isRunning = false;

            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {                
                bool isDown = (event.type == SDL_EVENT_KEY_DOWN);
                switch (event.key.key){
                    case SDLK_W:      input.forward = isDown; break;
                    case SDLK_S:      input.backward = isDown; break;
                    case SDLK_A:      input.left = isDown; break;
                    case SDLK_D:      input.right = isDown; break;  
                    case SDLK_LSHIFT:  input.lshift = isDown; break;                                                         
                    case SDLK_ESCAPE: 
                                if (isDown) {
                                    if (gameState == EditorState::Playing) {
                                        // Act exactly like the "Stop" button
                                        gameState = EditorState::Editor;
                                        needsSnap = true; 
                                    } else {
                                        // If already in Editor mode, close the engine
                                        isRunning = false; 
                                    }
                                }
                                break;
                }
            }

            if (isRightMouseButtonDown) {
                editorCamera.RotateCamera(mouseDeltaX * 0.1f, mouseDeltaY * 0.1f);
            }

        }

        if (requestCameraReset) {
            editorCamera.position = glm::vec3(0.0f, 5.0f, 15.0f);
            
            glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);    
            glm::vec3 dirToOrigin = glm::normalize(target - editorCamera.position);        
            // Force the internal vectors to recalculate immediately
            editorCamera.SetDirection(dirToOrigin);
            
            Log("Reset Camera to Origin");
            requestCameraReset = false; // Turn the flag off immediately
        }    
           
        if (isRightMouseButtonDown && (mouseDeltaX != 0 || mouseDeltaY != 0)) {
            editorCamera.RotateCamera(mouseDeltaX * 0.1f, mouseDeltaY * 0.1f);
            
            // CRITICAL: Reset deltas so they don't accumulate forever
            mouseDeltaX = 0;
            mouseDeltaY = 0;
        }
        
       

        float cameraSpeed = moveSpeed * 1.5f; // Move faster in editor mode            
        if (input.lshift) {
            cameraSpeed = moveSpeed * 5.0f; // Increase speed for Shift pressed
        } else {
            cameraSpeed = moveSpeed * 1.5f; // Default speed when Shift is not pressed
        }
        if (input.forward)  editorCamera.position += editorCamera.front * cameraSpeed * deltaTime;
        if (input.backward) editorCamera.position -= editorCamera.front * cameraSpeed * deltaTime;
        if (input.left)     editorCamera.position -= editorCamera.right * cameraSpeed * deltaTime;
        if (input.right)    editorCamera.position += editorCamera.right * cameraSpeed * deltaTime;        

        // Prepare for 3D Rendering
        glEnable(GL_DEPTH_TEST);        
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR: Framebuffer is not complete! Status: " << status << std::endl;
        }

        if (myShader.ID == 0) {
            std::cerr << "CRITICAL: Shader failed to compile! Check file paths." << std::endl;
        } 

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Sky blue   

        glViewport(0, 0, windowWidth, windowHeight);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = editorCamera.GetProjectionMatrix(currentAspectRatio);
        
        //Draw your 3D Scene
        myShader.use();                  
        myShader.setMat4("view", editorCamera.GetViewMatrix());
        myShader.setMat4("projection", projection);      
        
        // Iterate through every possible entity ID
        // Set the model matrix using your transform component (if it exists)
        glm::mat4 model;

        for (size_t e = 0; e < registry.renderables.size(); ++e) {
            if (!registry.hasRenderable[e]) continue; // Skip if no mesh to draw
            
            auto& renderable = registry.renderables[e];
            
            // Initialize identity matrix
            model = glm::mat4(1.0f);

            // Build the model matrix once using the transform
            if (registry.hasTransform[e]) {
                auto& t = registry.transforms[e];
                model = glm::translate(model, t.position);
                model = glm::rotate(model, glm::radians(t.rotation.x), glm::vec3(1,0,0));
                model = glm::rotate(model, glm::radians(t.rotation.y), glm::vec3(0,1,0));
                model = glm::rotate(model, glm::radians(t.rotation.z), glm::vec3(0,0,1));
                model = glm::scale(model, t.scale); 
            }                                                 
          
            if (registry.hasColor[e]) {
                myShader.setVec3("objectColor", registry.colors[e].color);
            }

            if (renderable.mesh != nullptr) {
                myShader.setBool("isVertexColor", false);
                
                if (registry.hasColor[e]) {
                    myShader.setVec3("objectColor", registry.colors[e].color);
                } else {
                    myShader.setVec3("objectColor", glm::vec3(1.0f));
                }

                // Draw the mesh   
                myShader.setMat4("model", model);
                myShader.setBool("isVertexColor", false);
                                
                // Set color
                myShader.setBool("isVertexColor", false);
                if (registry.hasColor[e]) {
                    myShader.setVec3("objectColor", registry.colors[e].color);
                } else {
                    myShader.setVec3("objectColor", glm::vec3(1.0f)); // Default white
                }
                
                // Render the mesh using its own draw function
                renderable.mesh->draw();
            }
        }

        // Draw X Axis (Red)
        debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(50,0,0));           
        // Draw Y Axis (Green)
        debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,50,0));       
        // Draw Z Axis (Blue)
        debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,0,50)); 

       
        // RenderSystem(registry, myShader, 0.0f); // Use your existing RenderSystem
        debugRenderer->Render(editorCamera.GetViewMatrix(), editorCamera.GetProjectionMatrix(currentAspectRatio), glm::vec3(1, 1, 1));
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        // ImGUI Window
        GetCurrentFrame(currentTime, lastTime, deltaTime);

        // Draw your UI
        DrawEditorUI(registry, selectedEntity, gameState, 0.0f, 60.0f);

        if (gameState == EditorState::Playing){
            RunGamePlay();
        }
        if (gameState == EditorState::Editor){
            SDL_SetWindowRelativeMouseMode(window, false);
            SDL_ShowCursor();
        }

        

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());         
        
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, count);
        GLenum err = glGetError(); 
        if (err != GL_NO_ERROR) {
            Log("OpenGL Error: " + std::to_string(err) + " detected in RenderLoop");
        }

        SDL_GL_SwapWindow(window);
    }

}

void RunGamePlay(){
    unsigned int fboID;
    unsigned int textureColorBufferID;
    unsigned int depthRenderBufferID;

    // Create FBO
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    // Create the texture we will render into
    glGenTextures(1, &textureColorBufferID);
    glBindTexture(GL_TEXTURE_2D, textureColorBufferID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBufferID, 0);

    // Create Depth Buffer
    glGenRenderbuffers(1, &depthRenderBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferID);

    // Check if FBO is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO

    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_HideCursor();

    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glViewport(0, 0, 800, 600);

    // Clear Screen
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    // exit gameplay
    // glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to screen
}


/*
int main(int argc, char* argv[]) {        
    SDL_Initializaton();    

    unsigned int fboID;
    unsigned int textureColorBufferID;
    unsigned int depthRenderBufferID;

    // Create FBO
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    // Create the texture we will render into
    glGenTextures(1, &textureColorBufferID);
    glBindTexture(GL_TEXTURE_2D, textureColorBufferID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBufferID, 0);

    // Create Depth Buffer
    glGenRenderbuffers(1, &depthRenderBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferID);

    // Check if FBO is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind FBO

 
    // SUCCESS! Query the GPU to prove we are running hardware acceleration
    Log("HT Game Engine Initialization Cleanly!");
    Log("VENDOR:   " + std::string((const char*)glGetString(GL_VENDOR)));
    Log("RENDERER: " + std::string((const char*)glGetString(GL_RENDERER)));
    Log("VERSION:  " + std::string((const char*)glGetString(GL_VERSION)));
            
    // ==========================================================
    // DYNAMIC SHADER ASSET LOADING & COMPILATION
    // ==========================================================
    Shader myShader("shaders/opengl_vertex.glsl", "shaders/opengl_fragment.glsl"); 
    Camera playerCamera;   

    // This camera won't move, it's just for the editor UI to show a static view of the scene
    Camera editorCamera;   

    // --- SETUP MESHES ---
    InitializeMeshes();
    debugRenderer = std::make_unique<DebugRenderer>();

    // --- SETUP ECS REGISTRY ---
    Registry registry;
    
    //SetupScene(registry); // Create entities and assign them components (e.g., Transform, Renderable)
      
    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();    
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460");  
    
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
       
    isRunning = true;
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
        float time = (float)SDL_GetTicks() / 1000.0f;

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
                if (!ImGui::GetIO().WantCaptureMouse){
                    if(gameState == EditorState::Editor){
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
                    case SDLK_ESCAPE: 
                                if (isDown) {
                                    if (gameState == EditorState::Playing) {
                                        // Act exactly like the "Stop" button
                                        gameState = EditorState::Editor;
                                        needsSnap = true; 
                                    } else {
                                        // If already in Editor mode, close the engine
                                        isRunning = false; 
                                    }
                                }
                                break;
                }
                
            }

            // Shoot a projectile when the left mouse button is clicked, Not in Debug Mode
            if (gameState == EditorState::Playing && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {                
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
                    registry.renderables[projectile] = { projectileMesh };
                    registry.hasRenderable[projectile] = true;
                }
            }
        }
        
        // Get the direction the camera is facing but ignore the Y axis. 
        glm::vec3 cameraForward = glm::normalize(glm::vec3(playerCamera.front.x, 0.0f, playerCamera.front.z));
        glm::vec3 cameraRight   = glm::normalize(glm::cross(cameraForward, playerCamera.up));

        if (gameState == EditorState::Playing) {
            // --- Gameplay Systems ---
            SDL_SetWindowRelativeMouseMode(window, true);
            SDL_HideCursor();

            glBindFramebuffer(GL_FRAMEBUFFER, fboID);
            glViewport(0, 0, 800, 600);

             // Clear Screen
            glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);           
          
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
         
            // Update the world state
            MovementSystem(registry, deltaTime);        
            
            // --- Camera follows player ---
            playerCamera.position = registry.transforms[playerEntity].position + glm::vec3(0.0f, 0.5f, 0.0f);    
            
            // Render your game here

            // Call the use function to activate the shader program for rendering graphics
            myShader.use();
            
            // Set a uniform variable in the shader program with the name "viewPos" and pass the current camera position
            myShader.setVec3("viewPos", playerCamera.position);        

            // Set a uniform variable in the shader program with the name "view" and pass the view matrix of the camera
            myShader.setMat4("view", playerCamera.GetViewMatrix());

            // Set a uniform variable in the shader program with the name "projection" and pass the projection matrix of the camera, adjusting for window aspect ratio
            myShader.setMat4("projection", playerCamera.GetProjectionMatrix(currentAspectRatio));
            
            myShader.setVec3("objectColor", glm::vec3(1.0f)); // White light

            RenderSystem(registry, myShader, time);           

            glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to screen

        } 
        else // Editor Systems --- 
        {           
            // Only move the editor camera, do not move the player
            // You could also add a system here that renders selection outlines (Gizmos)
            SDL_SetWindowRelativeMouseMode(window, false);
            SDL_ShowCursor();            
            
            // Snap if we just entered debug mode to see the player
            if(needsSnap){
                // Calculate vector from camera to player
                // get the reference to the player current position
                glm::vec3 playerPos = registry.transforms[playerEntity].position;
                
                playerCamera.position = playerPos + glm::vec3(0.0f, 0.5f, 0.0f);    

                // Define an offset for editorCamera
                glm::vec3 offset = glm::vec3(3.0f, 3.0f, 3.0f);
                editorCamera.position = playerPos + offset;

                // Calculate direction to look at the player
                glm::vec3 direction = glm::normalize(playerPos - editorCamera.position);

                // Update the camera's orientation to look at the player                                
                editorCamera.SetDirection(direction);
                needsSnap = false; // Snap done. 
            }

            float cameraSpeed = moveSpeed * 5.0f; // Move faster in debug mode            
            // In debug mode, we can move the camera freely like a spectator
            if (input.forward)  editorCamera.position += editorCamera.front * cameraSpeed * deltaTime;
            if (input.backward) editorCamera.position -= editorCamera.front * cameraSpeed * deltaTime;
            if (input.left)     editorCamera.position -= editorCamera.right * cameraSpeed * deltaTime;
            if (input.right)    editorCamera.position += editorCamera.right * cameraSpeed * deltaTime;
            
            
            //registry.hasRenderable[playerCameraEntity] = true;            
            //registry.renderables[playerCameraEntity].mesh = playerCameraMesh;
            //registry.hasRenderable[playerEntity] = true;     
            //registry.renderables[playerEntity].mesh = playerMesh;
            

           // Draw X Axis (Red)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(50,0,0));           
           // Draw Y Axis (Green)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,50,0));       
           // Draw Z Axis (Blue)
           debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,0,50));           

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
            
            // Rendering for Editor Camera Scene
            // Clear Screen for Editor
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, width, height); // Ensure full window viewport
            
            // Clear Screen
            glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 
            
            // Render the editor camera view of the scene
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            
            // Call the use function to activate the shader program for rendering graphics
            myShader.use();
        
            // Set a uniform variable in the shader program with the name "viewPos" and pass the current camera position
            myShader.setVec3("viewPos", editorCamera.position);        

            // Set a uniform variable in the shader program with the name "view" and pass the view matrix of the camera
            myShader.setMat4("view", editorCamera.GetViewMatrix());

            // Set a uniform variable in the shader program with the name "projection" and pass the projection matrix of the camera, adjusting for window aspect ratio
            myShader.setMat4("projection", editorCamera.GetProjectionMatrix(currentAspectRatio));
            myShader.setVec3("objectColor", glm::vec3(1.0f)); // White light
            RenderSystem(registry, myShader, time);
            
            
                 
        } // Editor System End 
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();     
        
        if (gameState == EditorState::Playing) {
            // Get the viewport dimensions
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 workPos = viewport->WorkPos;
            ImVec2 workSize = viewport->WorkSize;
            
            // Set the window size and position
            ImVec2 windowSize(800, 600);
            ImVec2 windowPos = ImVec2(workPos.x + (workSize.x - windowSize.x) * 0.5f, 
                                    workPos.y + (workSize.y - windowSize.y) * 0.5f);

            ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
            ImGui::Begin("Game Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
            
            // Display the texture
            ImGui::Image((void*)(intptr_t)textureColorBufferID, ImVec2(800, 600), ImVec2(0, 1), ImVec2(1, 0));            
            ImGui::End();
        }

        // Draw Editor UI
        if (gameState == EditorState::Editor) {            
            DrawEditorUI(registry, selectedEntity, gameState, deltaTime, fps);
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
void SetupScene(Registry &registry) {
    // Create Player
    playerEntity = registry.CreateEntity();
    registry.transforms[playerEntity].position = glm::vec3(0.0f, 0.5f, 0.0f);
    registry.hasTransform[playerEntity] = true;
    
    // Assign the GLOBAL mesh pointer directly
    registry.renderables[playerEntity].mesh = playerMesh; 
    registry.hasRenderable[playerEntity] = true;

    // Create player camera entity
    playerCameraEntity = registry.CreateEntity();
    registry.transforms[playerCameraEntity].position = glm::vec3(0.0f, 0.0f, 0.0f);
    registry.hasTransform[playerCameraEntity] = true;
    
    registry.renderables[playerCameraEntity].mesh = playerCameraMesh;
    registry.hasRenderable[playerCameraEntity] = false; 
    registry.colors[playerCameraEntity] = { glm::vec3(0.0f, 1.0f, 0.0f) };
    registry.hasColor[playerCameraEntity] = true;

    // Create Floor
    Entity floorEntity = registry.CreateEntity();
    registry.transforms[floorEntity] = { glm::vec3(0.0f, 0.0f, 0.0f) };
    registry.hasTransform[floorEntity] = true;
    
    registry.renderables[floorEntity].mesh = floorMesh;
    registry.hasRenderable[floorEntity] = true;
    registry.colors[floorEntity] = { glm::vec3(0.1f, 0.3f, 0.1f) };
    registry.hasColor[floorEntity] = true;

    // testCube 1
    Entity testCube = registry.CreateEntity();
    registry.transforms[testCube] = { glm::vec3(-1.0f, 1.0f, -10.0f) };
    registry.hasTransform[testCube] = true;
    registry.colors[testCube] = { glm::vec3(0.0f, 0.0f, 1.0f) };
    registry.hasColor[testCube] = true;
    
    registry.renderables[testCube].mesh = testCubeMesh; // Use global testCubeMesh
    registry.hasRenderable[testCube] = true;
    registry.rotations[testCube] = { 0.0f, glm::vec3(-1.0f, 0.5f, 0.0f), 0.1f };
    registry.hasRotation[testCube] = true;


    // testCube 2
    Entity testCube2 = registry.CreateEntity();
    registry.transforms[testCube2] = { glm::vec3(5.0f, 1.0f, 10.0f) };
    registry.hasTransform[testCube2] = true;
    registry.colors[testCube2] = { glm::vec3(1.0f, 0.0f, 0.0f) };
    registry.hasColor[testCube2] = true;
    
    registry.renderables[testCube2].mesh = testCubeMesh2; // Use global testCubeMesh2
    registry.hasRenderable[testCube2] = true;
    registry.rotations[testCube2] = { 0.0f, glm::vec3(0.0f, 0.5f, 0.0f), 0.1f };
    registry.hasRotation[testCube2] = true;
}

void SetupEntity(Registry& reg, std::shared_ptr<Mesh> mesh, glm::vec3 position, std::string name) {
    Entity e = reg.CreateEntity();

    // Set Name
    reg.names[e].name = name;
    reg.hasName[e] = true;

    // Set Transform
    reg.transforms[e] = { position };
    reg.hasTransform[e] = true;

    // Set Renderable (passing the mesh pointer)
    reg.renderables[e] = { mesh };
    reg.hasRenderable[e] = true;

    // Optional: Add default rotation or color
    reg.rotations[e] = { 0.0f, glm::vec3(0.0f, 1.0f, 0.0f), 20.0f };
    reg.hasRotation[e] = true;
}
*/