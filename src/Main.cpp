#include <glad/glad.h>  // CRITICAL: Always include GLAD before SDL3!

#include "ECS.h"
#include "Systems.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "CubeBuilder.h"
#include "DebugRenderer.h"
#include "MeshManager.h"
#include "SceneSerializer.h"

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

std::unique_ptr<DebugRenderer> debugRenderer;
std::vector<std::string> logMessages;

Camera editorCamera, playerCamera;  

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

void RunGamePlay(Registry& reg, Entity player, Camera& cam, InputState& input, float deltaTime);
void DrawEditorUI(Registry& registry, Entity& selectedEntity, EditorState& gameState, float deltaTime, float fps);

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

/*
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
}*/


// width: Total span along the X axis
// length: Total span along the Z axis
// step: The size of each individual grid square (e.g., 1.0f)
int SetupGrid(unsigned int& vao, unsigned int& vbo, float width, float step = 1.0f) {
    std::vector<float> vertices;
    
    float halfWidth = width / 2.0f;    

    // 1. Draw lines parallel to the X axis (spaced out along the Z axis)
    for (float z = -halfWidth; z <= halfWidth; z += step) {
        vertices.insert(vertices.end(), {-halfWidth, 0.0f, z,  halfWidth, 0.0f, z});
    }

    // 2. Draw lines parallel to the Z axis (spaced out along the X axis)
    for (float x = -halfWidth; x <= halfWidth; x += step) {
        vertices.insert(vertices.end(), {x, 0.0f, -halfWidth,  x, 0.0f, halfWidth});
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return (int)vertices.size() / 3; 
}

void Log(const std::string& message) {
    std::cout << message << std::endl;
    logMessages.push_back(message);
    
    // Keep only the last 100 messages to save memory
    if (logMessages.size() > 100) {
        logMessages.erase(logMessages.begin());
    }
}


void GetCurrentFrame(const float &deltaTime){
    static float displayFPS = 0.0f;
    static float timer = 0.0f;

    timer += deltaTime;
    // Update the display value only every 0.2 seconds
    if (timer >= 0.2f) {
        displayFPS = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
        timer = 0.0f;
    }

    ImGui::Begin("Performance Monitor");
    ImGui::Text("FPS: %.1f", displayFPS);

    ImGui::Separator();
    ImGui::Text("Camera Information");
    ImGui::Text("Position X: %.2f", editorCamera.position.x);
    ImGui::Text("Position Y: %.2f", editorCamera.position.y);
    ImGui::Text("Position Z: %.2f", editorCamera.position.z);                  
                    
    ImGui::Text("Pitch: %.2f", editorCamera.pitch);
    ImGui::Text("Yaw: %.2f", editorCamera.yaw);

    ImGui::End();
}

Entity playerID;
Entity CreatePlayer(Registry& registry) {
    Entity player = registry.CreateEntity();

    // Position & Transform
    registry.hasTransform[player] = true;
    registry.transforms[player].position = glm::vec3(0.0f, 1.0f, 0.0f);
    registry.transforms[player].scale = glm::vec3(0.5f, 1.0f, 0.5f); // Player dimensions

    // Rendering
    registry.hasRenderable[player] = true;
    registry.renderables[player].mesh = MeshManager::CreateNewCubeMesh();
    registry.renderables[player].color = glm::vec3(0.8f, 0.2f, 0.2f); // Give the player a distinct color
    
    // Movement / Logic components
    registry.hasVelocity[player] = true;
    registry.velocities[player].value = glm::vec3(0.0f);

    registry.hasName[player] = true;
    registry.names[player].name = "Player";

    return player;
}

void ResetPlayer(Registry& reg, Entity playerID) {
    reg.transforms[playerID].position = glm::vec3(0.0f, 1.0f, 0.0f);
    reg.velocities[playerID].value = glm::vec3(0.0f);
}

void ToggleGameState(EditorState& gameState, Registry &registry, const Entity &playerID) {
     if (gameState == EditorState::Editor) {
        gameState = EditorState::Playing;
        auto& pTransform = registry.transforms[playerID];
        playerCamera.position = pTransform.position + glm::vec3(0.0f, 1.6f, 0.0f);
        playerCamera.yaw = -90.0f;   // Look straight ahead
        playerCamera.pitch = 0.0f;   // Look at horizon                 
        playerCamera.UpdateCameraVectors();                    
        SDL_SetWindowRelativeMouseMode(window, true); // Lock mouse
    } else {
        gameState = EditorState::Editor;
        SDL_SetWindowRelativeMouseMode(window, false); // Unlock mouse
    }
}

unsigned int fboID, textureColorBufferID, depthRenderBufferID;

void InitGameplayFramebuffer() {
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);

    glGenTextures(1, &textureColorBufferID);
    glBindTexture(GL_TEXTURE_2D, textureColorBufferID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBufferID, 0);

    glGenRenderbuffers(1, &depthRenderBufferID);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferID);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferID);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main(int argc, char* argv[]) {        
    SDL_Initializaton();
    // --- ImGui Initialization ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();    
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460");      
    

    // Initialize Framebuffer ONCE before the loop
    InitGameplayFramebuffer();

    debugRenderer = std::make_unique<DebugRenderer>();
    Uint64 lastTime = SDL_GetTicks();
    Shader myShader("shaders/opengl_vertex.glsl", "shaders/opengl_fragment.glsl");     
    Shader debug_lineShader("shaders/debug_line_vertex.glsl", "shaders/debug_line_fragment.glsl");
    Shader gridShader("shaders/grid_vertex.glsl", "shaders/grid_fragment.glsl");
    SDL_Event event; 
    
    Registry registry;
    // --- Player is Special Entity ----
    Entity playerID = CreatePlayer(registry);   

    InputState input;     
    float moveSpeed = 1.00f;   
    editorCamera.position = glm::vec3(0.0f, 5.0f, 15.0f);
    
     // SUCCESS! Query the GPU to prove we are running hardware acceleration
    Log("HT Game Engine Initialization Cleanly!");
    Log("VENDOR:   " + std::string((const char*)glGetString(GL_VENDOR)));
    Log("RENDERER: " + std::string((const char*)glGetString(GL_RENDERER)));
    Log("VERSION:  " + std::string((const char*)glGetString(GL_VERSION)));
 

    unsigned int gridVAO, gridVBO;
    int gridCount; // grid count

    while (isRunning) {               
         // Calculate deltaTime for smooth movement regardless of frame rate
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert milliseconds to seconds
        lastTime = currentTime;                            

        // Calculate fresh every frame so it works even if the window is resized
        int width, height;

		// SDL_GetWindowSize(window, &width, &height);
        // Use this instead of SDL_GetWindowSize to handle high-DPI screens correctly
        SDL_GetWindowSizeInPixels(window, &width, &height);

        // Update the Viewport to match the actual pixel dimensions        
        glViewport(0, 0, width, height);        

        gridCount = SetupGrid(gridVAO, gridVBO, width);

        if (height == 0) height = 1; // Prevent division by zero            
        float currentAspectRatio = (float)width / (float)height; 
        float mouseDeltaX, mouseDeltaY;

        while (SDL_PollEvent(&event)) {              
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
			if (event.type == SDL_EVENT_MOUSE_WHEEL && gameState == EditorState::Editor) {				
				float zoomSpeed = 1.0f;
				editorCamera.position += editorCamera.GetForward() * (event.wheel.y * zoomSpeed);	
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

            // Change to Gameplay by Pressing F5
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F5) { 
                ToggleGameState(gameState, registry, playerID);                            
            }
            
            ImGui_ImplSDL3_ProcessEvent(&event);  
            
            if (isRightMouseButtonDown) {
                if (gameState == EditorState::Playing) {
                    playerCamera.RotateCamera(mouseDeltaX * 0.1f, mouseDeltaY * 0.1f);                    
                } else if (gameState == EditorState::Editor) {                  
                    editorCamera.RotateCamera(mouseDeltaX * 0.1f, mouseDeltaY * 0.1f);
                }
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
        
        // --- Logic Update ---
        if (gameState == EditorState::Playing) {
            RunGamePlay(registry, playerID, playerCamera, input, deltaTime);
        }      
  
        if (gameState == EditorState::Playing) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 
                        0, 0, width, height, 
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        
        if (gameState == EditorState::Editor) {
            // Editor Movement Logic (Your existing input code)
            if (input.forward)  editorCamera.position += editorCamera.front * cameraSpeed * deltaTime;
            if (input.backward) editorCamera.position -= editorCamera.front * cameraSpeed * deltaTime;
            if (input.left)     editorCamera.position -= editorCamera.right * cameraSpeed * deltaTime;
            if (input.right)    editorCamera.position += editorCamera.right * cameraSpeed * deltaTime;     
        }

        // --- Rendering ---
        // Pick camera based on state
        Camera* activeCam = (gameState == EditorState::Playing) ? &playerCamera : &editorCamera;

        // Use your FBO resolution (e.g., 1920x1080) for Playing, window size for Editor
        float aspect = (gameState == EditorState::Playing) 
                       ? (WINDOW_WIDTH/ WINDOW_HEIGHT) 
                       : ((float)width / (float)height);

        // Prepare for 3D Rendering        
        glClearColor(0.17f, 0.62f, 0.82f, 1.0f); // blue   
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);                    
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);        
              
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //Draw your 3D Scene
        myShader.use();                  
        myShader.setMat4("view", activeCam->GetViewMatrix());
        myShader.setMat4("projection", activeCam->GetProjectionMatrix(currentAspectRatio));             
          

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
                registry.renderables[e].mesh->draw();            
            }
        }          
               
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        // ImGUI Window
        GetCurrentFrame(deltaTime);

        // Draw your UI
        if (gameState == EditorState::Editor) {
            gridShader.use();
            gridShader.setMat4("view", activeCam->GetViewMatrix());
            gridShader.setMat4("projection", activeCam->GetProjectionMatrix(currentAspectRatio));  
            gridShader.setMat4("model", glm::mat4(1.0f)); 
            glBindVertexArray(gridVAO);
            glDrawArrays(GL_LINES, 0, gridCount);
          
            SDL_SetWindowRelativeMouseMode(window, false);
            SDL_ShowCursor();
            
            DrawEditorUI(registry, selectedEntity, gameState, 0.0f, 60.0f);   

            debug_lineShader.use();
            debug_lineShader.setMat4("view", activeCam->GetViewMatrix());
            debug_lineShader.setMat4("projection", activeCam->GetProjectionMatrix(currentAspectRatio));  
            debug_lineShader.setMat4("model", glm::mat4(1.0f)); 
    
            // Draw X Axis (Red)
            debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(50,0,0));           
            debugRenderer->Render(activeCam->GetViewMatrix(), activeCam->GetProjectionMatrix(currentAspectRatio), glm::vec3(1.0f, 0.0f, 0.0f)); // Red

            // Draw Y Axis (Green)
            debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,50,0));       
            debugRenderer->Render(activeCam->GetViewMatrix(), activeCam->GetProjectionMatrix(currentAspectRatio), glm::vec3(0.0f, 1.0f, 0.0f)); // Green

            // Draw Z Axis (Blue)
            debugRenderer->AddLine(glm::vec3(0,0,0), glm::vec3(0,0,50)); 
            debugRenderer->Render(activeCam->GetViewMatrix(), activeCam->GetProjectionMatrix(currentAspectRatio), glm::vec3(0.0f, 0.0f, 1.0f)); // Blue
        }           
         
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());         
              
        GLenum err = glGetError(); 
        if (err != GL_NO_ERROR) {
            Log("OpenGL Error: " + std::to_string(err) + " detected in RenderLoop");
        }

        SDL_GL_SwapWindow(window);
    }

}

void RunGamePlay(Registry& reg, Entity player, Camera& cam, InputState& input, float deltaTime) {        

    auto& transform = reg.transforms[player];
    float speed = 3.0f * deltaTime;

    if (input.forward)  transform.position += cam.front * speed;
    if (input.backward) transform.position -= cam.front * speed;
    if (input.left)     transform.position -= cam.right * speed;
    if (input.right)    transform.position += cam.right * speed;

    cam.position = reg.transforms[player].position + glm::vec3(0.0f, 1.6f, 0.0f);
    cam.UpdateCameraVectors(); // Ensure vectors update after position shift
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
    
    // ----- Top Menu Bar ------
    if (ImGui::BeginMainMenuBar()) {  
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SceneSerializer::SaveScene(registry, "world.scene");
                Log("Scene saved to world.scene");                                               
            }

            if (ImGui::MenuItem("Load Scene", "Ctrl+L")) {
                SceneSerializer::LoadScene(registry, "world.scene");
                
                playerID = (Entity)-1; 
                for (size_t i = 0; i < registry.hasName.size(); ++i) {
                    if (registry.hasName[i] && registry.names[i].name == "Player") {
                        playerID = (Entity)i;
                        Log("Player found and linked to ID: " + std::to_string(playerID));
                        break;
                    }
                }
                Log("Scene loaded from world.scene");
            }
            ImGui::Separator(); 
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                isRunning = false; // This will break the while(isRunning) loop in main.cpp
            }
            ImGui::EndMenu();
        }
        static bool showSavePopup = false;
        if (showSavePopup) {
            ImGui::Begin("Status", &showSavePopup);
            ImGui::Text("Scene Saved Successfully!");
            if (ImGui::Button("Close")) showSavePopup = false;
            ImGui::End();
        }
        

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
                Log("State changed to Playing");
                needsSnap = false;
            }
        } else {
            // Optional: Make "Stop" red to signify an active game
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button(" Stop ", ImVec2(buttonWidthPlay, 0))){ 
                gameState = EditorState::Editor; 
                Log("State changed to Editor Mode");
                needsSnap = true;
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndMainMenuBar();
    }   
    ImGui::Separator();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    // ------ Hierarchy ------
    ImGui::SetNextWindowPos(ImVec2(0, 30), ImGuiCond_Always); // ImGuiCond_FirstUseEver or ImGuiCond_Always
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, screenH - 20 - bottomPanelHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Scene Hierarchy", nullptr, flags)) {
             // Add Button
        if (ImGui::Button("Add New Entity", ImVec2(-1, 0))) {
            Entity newEnt = registry.CreateEntity();
        
            // 1. Create and get the mesh from your library
            std::shared_ptr<Mesh> cubeMesh = MeshManager::CreateNewCubeMesh();
            
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
            MeshManager::CleanupUnusedMeshes();
            
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


    // ------ INSPECTOR ------
    float inspectorWidthPercentage = 0.15f; // Adjust this value as needed
    float inspectorWidth = ImGui::GetIO().DisplaySize.x * 0.2f;
    ImGui::Separator();   
    inspectorWidth = ImGui::GetIO().DisplaySize.x * inspectorWidthPercentage; 
    ImGui::SetNextWindowPos(ImVec2(screenW - inspectorWidth, 30), ImGuiCond_Always); // ImGuiCond_FirstUseEver or ImGuiCond_Always
    ImGui::SetNextWindowSize(ImVec2(inspectorWidth, screenH), ImGuiCond_Always);  

    ImGui::Begin("Inspector", nullptr, flags); // Add flags
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
        
    // ----- Output Console -----
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