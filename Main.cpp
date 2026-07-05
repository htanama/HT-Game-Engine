#include <glad/glad.h>

#include <SDL3/SDL.h>
#include "core/Engine.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "editor/EditorLayer.h"
#include "core/Camera.h"
#include "core/ECS.h"
#include "core/Systems.h"

#define STB_IMAGE_IMPLEMENTATION
#include "utility/stb_image.h"

bool Engine::isRunning = true;
extern Entity selectedEntity;
extern Renderer renderer;
Registry registry;

EditorState gameState = EditorState::Editor;
static glm::vec3 dragOffset(0.0f);
static glm::vec3 initialEntityPosition(0.0f);
static glm::vec3 initialMouseWorldPos(0.0f);
bool isDragging = false;


int main(int argc, char* argv[]) {        
    
    SDL_Init(SDL_INIT_VIDEO);   
    
    SDL_Window* window = SDL_CreateWindow("HT Game Engine", 
        renderer.GetWindowWidth(), 
        renderer.GetWindowHeight(), 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    SDL_GLContext context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);  
   
    Shader myShader("shaders/opengl_vertex.glsl", "shaders/opengl_fragment.glsl");
    Shader gridShader("shaders/grid_vertex.glsl", "shaders/grid_fragment.glsl");

	EditorLayer editor;
    editor.Init(window, context);
    Logger::Log("Rendering Initialization Complete");
    
    Camera editorCamera;
    float original_speed = 7.0f;
    editorCamera.MovementSpeed = original_speed;
    editorCamera.position = glm::vec3(0.0f, 5.0f, 10.0f);

    SDL_Event event;
    Uint64 lastTime = SDL_GetTicks();          


	// Create test entity
Entity testEntity = registry.CreateEntity();
registry.AddTransform(testEntity, { glm::vec3(-5.0f, 0.0f, 0.0f), glm::vec3(0.0f) });
registry.AddVelocity(testEntity, glm::vec3(0.0f, -2.0f, 0.0f)); 
registry.names[testEntity] = {"player"};

registry.colors[testEntity].color = glm::vec3(1.0f, 1.0f, 1.0f); 
registry.hasColor[testEntity] = true;
registry.hasVelocity[testEntity] = true;
std::shared_ptr<Mesh> meshInstance = MeshManager::CreateNewCubeMesh();
registry.renderables[testEntity].mesh = meshInstance;
registry.hasRenderable[testEntity] = true;

	
    while (Engine::isRunning) {
        // Calculate deltaTime
        Uint64 currentTime = SDL_GetTicks();
        float deltaTime = static_cast<float>(currentTime - lastTime) / 1000.f; // covert from millisecond to second
        lastTime = currentTime;          

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) Engine::SetIsRunning(false);
			
			if (event.type == SDL_EVENT_KEY_DOWN) {
				if (event.key.scancode == SDL_SCANCODE_ESCAPE){
				    if (gameState == EditorState::Playing) {
				        gameState = EditorState::Editor;
				        Logger::Log("State changed to Editor Mode");
				    }
				}
			}
			
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                // Check if Right Mouse Button (SDL_BUTTON_RMASK) is pressed
                if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK) {
                    float xOffset = event.motion.xrel;
                    float yOffset = event.motion.yrel; // Inverted Y for standard FPS feel
                    editorCamera.RotateCamera(xOffset, yOffset); //
                }
            }

            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                float zoomSpeed = 3.0f;
                editorCamera.position += editorCamera.GetForward() * (event.wheel.y * zoomSpeed);
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
		
			// Perform raycasting to select object on the scene
			//if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
			//	if(event.button.button == SDL_BUTTON_LEFT) {
			//		ImGuiIO& io = ImGui::GetIO();
			//		if (!io.WantCaptureMouse) { // perform raycast if ImGui does not need the mouse
			//			float mx = (float)event.button.x;
			//			float my = (float)event.button.y;
			//		
			//			Ray ray = editorCamera.ScreenToWorldRay(mx, my, renderer.currentWindowWidth, renderer.currentWindowHeight);
			//			selectedEntity = PickEntity(ray, registry);
			//		
			//			if (selectedEntity != -1) {
			//				isDragging = true; // grab object
			//				std::cout << "You clicked entity: " << selectedEntity << std::endl;
			//			}
			//		}
			//	}
			//}

			//if (event.type == SDL_EVENT_MOUSE_MOTION) {
			//	if (isDragging && selectedEntity != -1) {
			//		// Move the entity based on mouse movement
			//		// We project the mouse position onto a plane at the object's current height (y)
			//		float height = registry.transforms[selectedEntity].position.y;
			//		
			//		Ray ray = editorCamera.ScreenToWorldRay((float)event.motion.x, (float)event.motion.y, 
			//												renderer.currentWindowWidth, renderer.currentWindowHeight);
			//		
			//		// This math finds where the ray hits the horizontal plane of the object
			//		float t = (height - ray.origin.y) / ray.direction.y;
			//		glm::vec3 newPos = ray.origin + (ray.direction * t);
			//		
			//		registry.transforms[selectedEntity].position = newPos;
			//	}
			//}
			

			// Mouse Down: Calculate offset to prevent snapping
			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				if(event.button.button == SDL_BUTTON_LEFT) {
					ImGuiIO& io = ImGui::GetIO();
					if (!io.WantCaptureMouse) {
						float mx = (float)event.button.x;
						float my = (float)event.button.y;

						Ray ray = editorCamera.ScreenToWorldRay(mx, my, renderer.currentWindowWidth, renderer.currentWindowHeight);
						selectedEntity = PickEntity(ray, registry);

						if (selectedEntity != -1) {
							isDragging = true;
							
							// Store the starting state
						    initialEntityPosition = registry.transforms[selectedEntity].position;
							
							// Calculate offset: Where does the ray hit the plane passing through the object?
							float dist;
							glm::vec3 planeNormal = -editorCamera.GetForwardVector(); // Plane faces camera																				
													
							Ray ray = editorCamera.ScreenToWorldRay((float)event.button.x, (float)event.button.y, 
                                           renderer.currentWindowWidth, renderer.currentWindowHeight);                                      																												
							// Calculate the point on the plane at the moment of click
						    RayIntersectsPlane(ray, initialEntityPosition, planeNormal, dist);	 
							
							// FIX: Assign to the global variable so MOUSE_MOTION can use it!
						    initialMouseWorldPos = ray.origin + (ray.direction * dist);															
							dragOffset = initialEntityPosition - initialMouseWorldPos;
							
						}
					}
				}
			}

			// Mouse Motion: Use the same camera-facing plane logic
			if (event.type == SDL_EVENT_MOUSE_MOTION) {
				// Check WantCaptureMouse here too, so we don't move objects if over a UI window
				ImGuiIO& io = ImGui::GetIO();
				if (isDragging && selectedEntity != -1 && !io.WantCaptureMouse) {
					Ray ray = editorCamera.ScreenToWorldRay((float)event.motion.x, (float)event.motion.y,
														   renderer.currentWindowWidth, renderer.currentWindowHeight);
					
					float dist;
					glm::vec3 planeNormal = -editorCamera.GetForwardVector();

					// We use the initial position as the base for the plane to prevent "snapping"
					if (RayIntersectsPlane(ray, initialEntityPosition, planeNormal, dist)) {			    
						// This is the raw world position the mouse is pointing at
						glm::vec3 currentMouseWorldPos = ray.origin + (ray.direction * dist);
						
						// Calculate how much the mouse moved from the starting click position
						glm::vec3 mouseDelta = currentMouseWorldPos - initialMouseWorldPos;
						
						// Apply movement: Start position + delta + the initial offset (to keep it from snapping)
						glm::vec3 newPos = initialEntityPosition + mouseDelta;
						
						glm::vec3& pos = registry.transforms[selectedEntity].position;
						if (!editor.IsLockedX()) pos.x = newPos.x;
						if (!editor.IsLockedY()) pos.y = newPos.y;
						if (!editor.IsLockedZ()) pos.z = newPos.z;						
					
					}
				}
			}	

			if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
				if(event.button.button == SDL_BUTTON_LEFT){
					isDragging = false; // Release the object
				}
			}	
			

		}
			              
        // WASD Movement (Polling for continuous input)
        const bool* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) editorCamera.ProcessKeyboard(FORWARD, deltaTime);
        if (state[SDL_SCANCODE_S]) editorCamera.ProcessKeyboard(BACKWARD, deltaTime);
        if (state[SDL_SCANCODE_A]) editorCamera.ProcessKeyboard(LEFT, deltaTime);
        if (state[SDL_SCANCODE_D]) editorCamera.ProcessKeyboard(RIGHT, deltaTime);
        
        if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) { // false means "do not repeat"
                SceneSerializer::SaveScene(registry, "world.scene");                
            }
        }
        
        if (state[SDL_SCANCODE_LSHIFT]) 
            editorCamera.MovementSpeed = original_speed + 4.0f;
        else {
            editorCamera.MovementSpeed = original_speed;
        }	
		
		if (gameState == EditorState::Playing){
				MovementSystem(registry, deltaTime);
		}
	
        float currentAspectRatio = (float)renderer.currentWindowWidth / renderer.currentWindowHeight;        

        // Render Game Scene
        renderer.DrawClearScreen(0.45f, 0.81f, 0.97f, 1.0f);        
        
        // Render solid surface 
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Reset pixels to background color and resets distance to draw obj correctly front to back (depth buffer)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // render objects closer to the camera appear in front of objects further away
        glEnable(GL_DEPTH_TEST);
        
		// Clear and Render the scene (Used by both modes)
		renderer.DrawClearScreen(0.45f, 0.81f, 0.97f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		myShader.use();
		myShader.setMat4("view", editorCamera.GetViewMatrix());
		myShader.setMat4("projection", editorCamera.GetProjectionMatrix(currentAspectRatio));
		RenderSystem(registry, myShader);


		// Only draw the grid when in Editor mode
		if (gameState == EditorState::Editor) {		
			editor.Draw(editorCamera, gridShader, currentAspectRatio);
		}
	
		// 2. UI Rendering Logic (ALWAYS run this, regardless of state)
		editor.Begin();
		editor.Draw(editorCamera); // Handles the "Game View" window[cite: 17]
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
