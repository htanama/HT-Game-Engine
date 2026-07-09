#pragma once

#include <memory>
#include "core/Engine.h"
#include "core/Camera.h"
#include "core/ECS.h"
#include "utility/3DShapeBuilder.h"
#include "utility/MeshManager.h"
#include "core/Systems.h"
#include "utility/SceneSerializer.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h" // required for ImGui::DockBuilder function
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <SDL3/SDL.h>

extern Registry registry;
static Entity selectedEntity;
bool requestCameraReset;
Renderer renderer;
int* keyToRebind = nullptr; 

class EditorLayer {
private: 
    std::vector<float> gridVertices;
    unsigned int m_gridVAO = 0;
    unsigned int m_gridVBO = 0;
    int m_gridCount; // grid count
    int m_width = 0, m_height = 0;
	bool showSavePopup = false;

	SDL_Window* m_window = nullptr; 

	 // Axis Lock States
    bool m_lockX = false;
    bool m_lockY = false;
    bool m_lockZ = false;
    bool showReticle = false;

    int SetupGrid(unsigned int& vao, unsigned int& vbo, float width, float step = 1.0f) {
        std::vector<float> vertices;

        float halfWidth = width / 2.0f;

        // Clean up old resources if they exist
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }

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
	
public:  

	bool isWaitingForKey = false;
    
	~EditorLayer() {
        glDeleteVertexArrays(1, &m_gridVAO);
        glDeleteBuffers(1, &m_gridVBO);
    }

    void Init(SDL_Window* window, SDL_GLContext context) {
        m_window = window;
		ImGui::CreateContext();
    
        // Enable the Docking Feature
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.2f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // add keyboard navigation 
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 410");                

        SDL_GetWindowSizeInPixels(window, &m_width, &m_height);
        glViewport(0, 0, m_width, m_height);
        m_gridCount = SetupGrid(m_gridVAO, m_gridVBO, m_width);        
      
    }
	
	void SetRebindKey(SDL_Keycode key) {
		if (isWaitingForKey && keyToRebind) {
			*keyToRebind = key;
			isWaitingForKey = false;
		}
	}

    void Begin() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }
	
	// Callback for loading a texture path
	static void TextureCallback(void* userdata, const char* const* filelist, int filter) {
		if (filelist && *filelist) {
			if (selectedEntity != -1) {
				std::string selectedPath = *filelist;
				
				// Update the path in the registry
				registry.textures[selectedEntity].path = selectedPath;
				
				// Perform the load and application automatically
				TextureComponent textureComponent;
				textureComponent.textureID = MeshManager::LoadTexture(selectedPath);
				textureComponent.useTexture = true; 
				textureComponent.path = selectedPath;
				
				registry.AddTexture(selectedEntity, textureComponent);
				registry.hasTexture[selectedEntity] = true;
				
				Logger::Log("Texture automatically applied: " + selectedPath);
			}
		}
	}

	static void SaveSceneCallback(void* userdata, const char* const* filelist, int filter) {
		if (filelist && *filelist) {
			EditorLayer* instance = static_cast<EditorLayer*>(userdata);
			std::string filename = *filelist;
		
			if (filename.length() < 6 || filename.substr(filename.length() - 6) != ".scene") {
				filename += ".scene";
			}

			// Use global registry
			SceneSerializer::SaveScene(registry, filename);
			Logger::Log("Scene saved to: " + filename);
			
			// This variable is local to the function in Draw(), 
			// to set a flag in the class, add 'bool showSavePopup' to your class members
			instance->showSavePopup = true; 
		}
	}

	// Callback for loading a scene
	static void LoadSceneCallback(void* userdata, const char* const* filelist, int filter) {
		if (filelist && *filelist) {
			SceneSerializer::LoadScene(registry, *filelist);
    	    Logger::Log("Scene loaded from: " + std::string(*filelist));
		}
	}	

	// Public methods use standard naming without 
    bool IsLockedX() const { return m_lockX; }
    bool IsLockedY() const { return m_lockY; }
    bool IsLockedZ() const { return m_lockZ; }

	// Helper to be used inside Draw() for cleaner UI
	void DrawTransformUI(Transform& t) {
        ImGui::Text("Position");
        
        // Use checkboxes to update the private state
        ImGui::Checkbox("Lock X", &m_lockX); ImGui::SameLine();
        ImGui::Checkbox("Lock Y", &m_lockY); ImGui::SameLine();
        ImGui::Checkbox("Lock Z", &m_lockZ);

        ImGui::Columns(3, nullptr, false);
        ImGui::Text("X"); ImGui::NextColumn();
        ImGui::Text("Y"); ImGui::NextColumn();
        ImGui::Text("Z"); ImGui::NextColumn();
        ImGui::Columns(1);

        ImGui::SetNextItemWidth(-FLT_MIN);

	}

    void DrawReticle() {
        // Use the Foreground DrawList to render on top of everything
        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        // Get the window position and size of your "Scene" window 
        // (Ensure you pass or have access to the Scene window's specific position)
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();

        // Calculate center
        ImVec2 center = ImVec2(pos.x + (size.x * 0.5f), pos.y + (size.y * 0.5f));

        float len = 25.0f;
        float thickness = 3.0f; 
        ImU32 color = ImColor(0, 255, 0, 255); 

        drawList->AddLine(ImVec2(center.x - len, center.y), ImVec2(center.x + len, center.y), color, thickness);
        drawList->AddLine(ImVec2(center.x, center.y - len), ImVec2(center.x, center.y + len), color, thickness);
    }

	
    void Draw(Camera &editorCamera) 
    {      

        // Dynamically get the size of the current UI window, not the whole application
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
       
        // 1. GLOBAL MENU BAR (Must be outside the DockSpace Host)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
		        if (ImGui::MenuItem("New Scene", "Ctrl+N")) { 
					Logger::Log("New Scene"); 

				    // Before calling ClearScene()
					for (size_t i = 0; i < registry.textures.size(); ++i) {
				        if (registry.hasTexture[i]) {
            				MeshManager::RemoveTexture(registry.textures[i].textureID); // Free GPU memory
        				}
    				}
					registry.ClearScene(); // Now safely clear the component data[cite: 10]
				
				 	// 2. Force the MeshManager to remove all meshes that are no longer referenced
					MeshManager::meshLibrary.clear(); // Clear the library
					MeshManager::CleanupUnusedMeshes(); // Ensure GPU/Memory cleanup
				
					// 3. Reset selection
					selectedEntity = -1;
				
					Logger::Log("New Scene created. All entities and meshes cleared."); 

				}
                if (ImGui::MenuItem("Autosave", "Ctrl+S")) { 
                    std::string filename = "autosave_file.scene";
					SceneSerializer::SaveScene(registry, filename);
                    Logger::Log("Scene saved to: " + filename);
				}               	

        
	        	if (ImGui::MenuItem("Save Scene As..", "Ctrl+S")) {   
					// Only open the dialog here. Do not save yet!
					SDL_ShowSaveFileDialog(SaveSceneCallback, this, m_window, nullptr, 0, nullptr);
				}               	

 
                if (ImGui::MenuItem("Load Scene", "Ctrl+L")) {
                	//SceneSerializer::LoadScene(registry, "world.scene");
					SDL_ShowOpenFileDialog(LoadSceneCallback, this, m_window, nullptr, 0, nullptr, false);
                }
                
                if (ImGui::MenuItem("Exit")) { Engine::SetIsRunning(false); }

                ImGui::EndMenu();
            }

            float buttonWidthOrigin = 100.0f;
            float buttonWidthPlay = 120.0f;
            float spacing = 800.0f; 
            float totalGroupWidth = buttonWidthOrigin + spacing + buttonWidthPlay;

            ImGui::SameLine(0, spacing); // Keep them on the same line

            if (ImGui::Button(" 2D ", ImVec2(buttonWidthOrigin, 0))) {
                // TODO: Change to 2D Scene
				ImGui::OpenPopup("2D Scene");				
            }
			
			ImGui::SetNextWindowSize(ImVec2(600,400), ImGuiCond_Always);
            if (ImGui::BeginPopupModal("2D Scene", nullptr,
                           ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Do you want to switch to 2D scene?");
				ImGui::Separator();

				if (ImGui::Button("Yes"))
				{
					//registry.DestroyEntity(entityToDelete);
					//entityToDelete = (Entity)-1;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();

				if (ImGui::Button("No"))
				{
					//entityToDelete = (Entity)-1;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

         


            if (ImGui::Button(" 3D ", ImVec2(buttonWidthOrigin, 0))) {
                // TODO: Change to 3D Scene
            }

            // State-based button
            if (gameState == EditorState::Editor) {
                if (ImGui::Button(" Play ", ImVec2(buttonWidthPlay, 0))) {
                    gameState = EditorState::Playing;
                    Logger::Log("State changed to Playing");
                }
            }
            else {
            	  // Optional: Make "Stop" red to signify an active game
            	   ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            	   if (ImGui::Button(" Stop ", ImVec2(buttonWidthPlay, 0))) {
            		   gameState = EditorState::Editor;
					   Logger::Log("State changed to Editor Mode");			
            	  }                                         
            	  ImGui::PopStyleColor();
            }

            ImGui::EndMainMenuBar();            


        }            


        // Create a dockspace in main viewport, where central node is transparent.
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		// Build a FIXED default layout the first time this run
		static bool dockLayoutBuilt = false;
		if(!dockLayoutBuilt){
			dockLayoutBuilt = true;
			
            // Wipe out any existing layout for this dockspace node and start fresh
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            // Split off a left dock (20%) for the Scene panel
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(
            dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);

            // Split off a right dock (22%) for the Inspector panel
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(
            dockspace_id, ImGuiDir_Right, 0.22f, nullptr, &dockspace_id);

            // Split off a bottom dock (25% of what's left) for the Output Console
            ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(
                dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

            // Whatever remains (dockspace_id) is the central viewport (your 3D scene view)

            // Assign each window to its designated dock node
            ImGui::DockBuilderDockWindow("Scene", dock_left_id);if (gameState == EditorState::Playing) {
            // Stop the game and return to Editor mode
            gameState = EditorState::Editor; 
            Logger::Log("Game exited to Editor mode via Window Close");
        }
            ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
            ImGui::DockBuilderDockWindow("Output Console", dock_bottom_id);

            ImGui::DockBuilderFinish(dockspace_id);
			
		}

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking | 
                                    ImGuiWindowFlags_NoTitleBar | 
                                    ImGuiWindowFlags_NoCollapse | 
                                    ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | 
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

        
        ImGui::Begin("Scene"); // ImGuiCond_FirstUseEver           
            

            ImGui::BeginChild("HierarchyChild", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), true);
            ImGui::Text("Hierarchy");
            
            if (ImGui::Button("Add New Entity", ImVec2(-1, 0))) {
                Entity newEnt = registry.CreateEntity();
                
                // Create and get the mesh from your meshLibrary
                std::shared_ptr<Mesh> meshInstance = MeshManager::CreateNewCubeMesh();

                // Error handling to catch if it is failed to create cubeMesh
                if (!meshInstance) {
                    Logger::Log("Failed to create new cube mesh. ");
                }

                // Register Component - Pass the shared_ptr from the library
                registry.renderables[newEnt].mesh = meshInstance;
                registry.hasRenderable[newEnt] = true;

                registry.AddTransform(newEnt, { glm::vec3(0.0f), glm::vec3(0.0f) });
                registry.hasTransform[newEnt] = true;

                registry.AddColor(newEnt, { glm::vec3(1.0f, 1.0f, 1.0f) });
                registry.colors[newEnt].color = glm::vec3(1.0f, 1.0f, 1.0f); // Default white color
                registry.hasColor[newEnt] = true;

                registry.names[newEnt] = { "New Cube" };
                registry.hasName[newEnt] = true;


                Logger::Log("Added New Entity and total number of entities: " + std::to_string(registry.GetEntityCount()));
            }             
                      
            // to check the correct entity to rename 			
            static Entity renamingEntity = (Entity)-1;
            static char nameBuffer[64] = "";

            // to check the correct entity to delete
            static Entity entityToDelete = (Entity)-1;
			
            bool isDeletePopupWindow = false;

			// For loop search each entity. All entity must have Transform
            for (size_t index = 0; index < registry.hasTransform.size(); ++index) { 
                if (!registry.hasTransform[index]) continue;			
			
                // ImGui::PushID() prevent ID conflicts by pushing 
                // a unique identifier onto the global ID stack.
                ImGui::PushID((int)index);
                // Retrieve name
                std::string entityName = registry.hasName[index] ? registry.names[index].name : "Object " + std::to_string(index);
                               
				// Logic: If we are currently renaming THIS specific entity
                if (renamingEntity == (Entity)index) {
                    ImGui::SetKeyboardFocusHere();
                    if (ImGui::InputText("##rename", nameBuffer, sizeof(nameBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {

                        registry.names[index].name = std::string(nameBuffer);
                        renamingEntity = (Entity)-1; // Exit rename mode
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) renamingEntity = (Entity)-1;
                }
                else {
                    // Normal display mode
                    if (ImGui::Selectable(entityName.c_str(), selectedEntity == (Entity)index,
						ImGuiSelectableFlags_SpanAvailWidth | 
    					ImGuiSelectableFlags_AllowOverlap | 
						ImGuiSelectableFlags_SelectOnNav)) 
					{
                        selectedEntity = (Entity)index;
                    }

                    // Trigger rename on double-click OR right-click menu
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        renamingEntity = (Entity)index;
                        strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
                    }                    
                    
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Rename")) {
                            renamingEntity = (Entity)index;
                            strncpy(nameBuffer, entityName.c_str(), sizeof(nameBuffer));
                        }
                        if (ImGui::MenuItem("Copy")) {
                            registry.CopyEntity((Entity)index);
                            Logger::Log("Entity copied.");
                        }
                        if (ImGui::MenuItem("Delete")) {
                            // this window is to assign true value of isDeletePopupWindow
                            // so the ImGui popup window can be shown due to PopupContext conflict					        
                            isDeletePopupWindow = true;							
                       	}                         									
                        ImGui::EndPopup();                                                                
                        
                    }

                }// End of else                            
                if(isDeletePopupWindow){
                    ImGui::OpenPopup("Delete Entity");
                    isDeletePopupWindow = false;
                }
                // Delete Entity Popup Window
                if (ImGui::BeginPopupModal("Delete Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Do you want to delete: %s", registry.names[index].name.c_str());
                    ImGui::Separator();
                    
                    // Define explicit button widths (or use ImGui::CalcTextSize)
                    float button_width = 80.0f; 
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    // Calculate the combined total width of both buttons and the space between them
                    float total_width = (button_width * 2.0f) + spacing;
                    // Calculate the starting X position to center the block
                    float available_width = ImGui::GetContentRegionAvail().x;
                    float start_x = (available_width - total_width) * 0.5f;
                    // Move the cursor and render the buttons
                    ImGui::SetCursorPosX(start_x);

                    if (ImGui::Button("Yes", ImVec2(button_width, 0.0f)))
                    {                        
                        entityToDelete = (Entity)index;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("No", ImVec2(button_width, 0.0f)))
                    {                        
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }// End ImGui::BeginPopupModal                   

                ImGui::PopID();
            }	            
            // this is the logic to remove the selected entity
			if (entityToDelete != (Entity)-1) { 
                RequestDeleteEntity(registry, entityToDelete);
                MeshManager::CleanupUnusedMeshes();

                // Reset selection if we deleted the currently selected one
                if (selectedEntity == entityToDelete) {
                    selectedEntity = (Entity)-1;
                }
                registry.SubtractEntityCount();
                Logger::Log("Entity ID " + std::to_string(entityToDelete) + " components cleared.");
                Logger::Log("Total entities: " + std::to_string(registry.GetEntityCount()));
                entityToDelete = (Entity)-1; // Reset target
            }
            ImGui::EndChild(); // end of Scene Hierarchy     


            ImGui::BeginChild("FileChild", ImVec2(0, 0), true);
            ImGui::Text("File System");
            ImGui::EndChild();
        ImGui::End();
        
        
        
        ImGui::Begin("Inspector");
 			ImGui::Separator();
			if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
				ImGui::OpenPopup("AddComponentPopup");
			}

			if (ImGui::BeginPopup("AddComponentPopup")) {
				ImGui::Text("Select Component");
				ImGui::Separator();

				// List components the entity DOES NOT yet have
				if (!registry.hasVelocity[selectedEntity]) {
					if (ImGui::Selectable("Velocity")) {
						registry.hasVelocity[selectedEntity] = true;
						// Initialize with default values if necessary
						registry.velocities[selectedEntity] = { glm::vec3(0.0f) };
					}
				}
		
				if (!registry.hasLifetime[selectedEntity]) {
					if (ImGui::Selectable("Lifetime")) {
						registry.hasLifetime[selectedEntity] = true;
						registry.lifetimes[selectedEntity] = { 60.0f };
					}
				}

				if (!registry.hasMesh[selectedEntity]) {
                	if (ImGui::Selectable("Mesh Component")) {
                        MeshComponent newComp(MeshType::Cube);
                        registry.hasMesh[selectedEntity] = true;                        
                        registry.AddMeshComponent(selectedEntity, newComp);
                    }
                }
				
				if (!registry.hasPlayerController[selectedEntity]) {
					if (ImGui::Selectable("Player Controller")) {
						registry.AddPlayerController(selectedEntity, 5.0f); // Set a default speed
					}
				}

				ImGui::EndPopup();
			}
	   
			ImGui::Separator();

			if (ImGui::Button("Origin 0,0,0")) {
                requestCameraReset = true;
            }
            if (requestCameraReset) {
                editorCamera.position = glm::vec3(0.0f, 5.0f, 15.0f);

                glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
                glm::vec3 dirToOrigin = glm::normalize(target - editorCamera.position);
                // Force the internal vectors to recalculate immediately
                editorCamera.SetDirection(dirToOrigin);

                Logger::Log("Reset Camera to origin");  
                requestCameraReset = false; // Turn the flag off immediately
            }
				
			if (selectedEntity != -1) {
				if(selectedEntity < registry.names.size()){
					if (registry.hasName[selectedEntity]) {
						// Display the name as read-only text
						ImGui::Text("Entity Name:");
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", registry.names[selectedEntity].name.c_str());
					}
				}

				// You could show the ID too, which is also read-only
				ImGui::Text("Entity ID: %d", (int)selectedEntity);
			} 

            ImGui::Text("Transform Position");      
            if (selectedEntity != -1 && selectedEntity < registry.transforms.size() && registry.hasTransform[selectedEntity]) {               
                Transform& t = registry.transforms[selectedEntity];
                // --- POSITION ---               
                ImGui::Columns(3, nullptr, false);
                ImGui::Text("X"); ImGui::NextColumn();
                ImGui::Text("Y"); ImGui::NextColumn();
                ImGui::Text("Z"); ImGui::NextColumn();
                ImGui::Columns(1); // Close columns so the sliders aren't forced into them
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::DragFloat3("##Position", &t.position.x, 0.1f);
                
                ImGui::Separator();

                // --- LOCATE / RESET ---
                if (ImGui::Button("Locate Entity")) {
                    // Place the camera a short distance behind/above the entity,
                    // then use the Camera's own SetDirection() to aim at it.
                    // (SetDirection handles the pitch sign convention AND calls
                    // UpdateCameraVectors() so 'front' is refreshed immediately.)
                    glm::vec3 offset(0.0f, 2.0f, 5.0f);
                    editorCamera.position = t.position + offset;
                    editorCamera.SetDirection(t.position - editorCamera.position);
                }

                ImGui::Separator();	
                
                // --- ROTATION ---
                ImGui::Text("Rotation");
                ImGui::Columns(3, nullptr, false);
                ImGui::Text("X"); ImGui::NextColumn();
                ImGui::Text("Y"); ImGui::NextColumn();
                ImGui::Text("Z"); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::DragFloat3("##Rotation", &t.rotation.x, 1.0f);
                
                ImGui::Separator();
                // --- SCALE ---
                ImGui::Text("Scale");
                ImGui::Columns(3, nullptr, false);
                ImGui::Text("X"); ImGui::NextColumn();
                ImGui::Text("Y"); ImGui::NextColumn();
                ImGui::Text("Z"); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::DragFloat3("##Scale", &t.scale.x, 0.5f);
                
                ImGui::Separator();
                DrawTransformUI(t);
            
                static bool checked = false;
                ImGui::Checkbox("Wireframe", &checked);
                // SAFE GUARD: Check if the index is valid BEFORE doing anything
                if (selectedEntity >= 0 && selectedEntity < (int)registry.renderables.size()) {
                    // if checkbox is on, change the mesh to wire frame globally not individual model.
                    // find entity that can be render on the screen to see if it is wireframe or fill polygon
                    for (Renderable& object : registry.renderables) {
                        // Find the object
                        if (&object == &registry.renderables[selectedEntity]) {
                            // Assign the value (this is the "toggle")
                            // If checked is true, object becomes true. If checked is false, it becomes false.
                            object.isWireframe = checked;
                            // Stop searching
                            break;
                        }

                    }
                }

                if (selectedEntity != -1 && selectedEntity < registry.colors.size() && registry.hasColor[selectedEntity]) {
                    ColorComponent& c = registry.colors[selectedEntity];

                    // ColorEdit3 takes a float array (a pointer to the first of 3 floats)
                    if (ImGui::ColorEdit3("Object Color", &c.color.x)) {
                        // This block runs automatically whenever the user changes the color.
                        // If your renderer is already using registry.colors[e].color
                        // in its main loop, this will work instantly!
                    }
                }
            
            

                ImGui::Separator();
                ImGui::Text("Texture Settings");        
                // Create a buffer for the file path (static so it persists)
                char pathBuffer[128] = "assets/wood.png"; 
                    
                if (selectedEntity != -1) {
                    // 1. If we have a valid selection, update the buffer from the entity's data
                    if (registry.hasTexture[selectedEntity]) {
                        std::string path = registry.textures[selectedEntity].path;
                        strncpy(pathBuffer, path.c_str(), sizeof(pathBuffer));
                    } else {
                        // Fallback for entities without textures
                        strncpy(pathBuffer, "No texture", sizeof(pathBuffer));
                    }

                    // 2. Render the UI
                    if (ImGui::InputText("Path", pathBuffer, sizeof(pathBuffer))) {
                        // 3. Optional: If the user types in the box, update your registry component here
                        if (registry.hasTexture[selectedEntity]) {
                            registry.textures[selectedEntity].path = std::string(pathBuffer);
                        }
                    }
                }

                if (ImGui::Button("Browse...")) {					
                    SDL_ShowOpenFileDialog(TextureCallback, this, m_window, nullptr, 0, nullptr, false);
                    Logger::Log("Browse for texture files: ");
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Apply Texture")) {
                    TextureComponent textureComponent;
                    textureComponent.textureID = MeshManager::LoadTexture(pathBuffer);
                    //textureComponent.textureID = MeshManager::LoadTextureToOpenGL(pathBuffer);
                    textureComponent.useTexture = true; 
                    textureComponent.path = pathBuffer; // Save the asset path so SceneSerializer can reload it later
                    registry.AddTexture(selectedEntity, textureComponent);
                    registry.hasTexture[selectedEntity] = true;					

                }
            
            
                ImGui::SameLine();

                // 3. Remove Texture Button
                if (ImGui::Button("Remove")) {
                    if (selectedEntity != -1 && registry.hasTexture[selectedEntity]) {
                        // 1. Tell MeshManager to handle the GPU cleanup
                        // We pass the ID so it can decrement references or delete the texture object
                        MeshManager::RemoveTexture(registry.textures[selectedEntity].textureID);
                        
                        // 2. Clear the component data
                        registry.textures[selectedEntity].useTexture = false;
                        registry.hasTexture[selectedEntity] = false;
                        registry.textures[selectedEntity].textureID = 0; // Reset ID to 0
                        
                        Logger::Log("Texture removed from entity " + std::to_string(selectedEntity));
                    }
                }

            
            

                if (ImGui::Checkbox("enable collision", &registry.physics[selectedEntity].isEnabled)) {
                    // AddPhysics both resizes the physics vector safely and sets
                    // hasPhysics[selectedEntity] = true, so MovementSystem's
                    // (hasPhysics && isEnabled) check actually passes.
                    registry.AddPhysics(selectedEntity, { registry.physics[selectedEntity].isEnabled });
                }

                // This is where the component will be added after the adding component selection is chosen. 
                // Adding the Velocity UI to the Inspector
                if (registry.hasVelocity[selectedEntity]) {
                    ImGui::Separator();
                    ImGui::Text("Velocity Settings");
                    
                    // Linear Velocity
                    ImGui::Text("Linear:");
                    ImGui::DragFloat3("##Linear", &registry.velocities[selectedEntity].linear.x, 0.1f);
                    
                    // Angular Velocity
                    ImGui::Text("Angular (Deg/s):");
                    ImGui::DragFloat3("##Angular", &registry.velocities[selectedEntity].angular.x, 1.0f);
                    
                    if (ImGui::Button("Remove Velocity")) {
                        registry.hasVelocity[selectedEntity] = false;
                    }
                }

                // Adding the LifeTimeComponent
                if (registry.hasLifetime[selectedEntity]) {
                    if (ImGui::CollapsingHeader("Lifetime Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                        // This formats the float to show two decimal places
                        ImGui::Text("Remaining Time: %.2f s", registry.lifetimes[selectedEntity].remainingTime);
                        
                        // DragFloat allows you to manually adjust the time if needed
                        ImGui::DragFloat("##lifetime", &registry.lifetimes[selectedEntity].remainingTime, 0.1f, 0.0f, 100.0f);
                        
                        if (ImGui::Button("Remove Lifetime")) {
                            registry.hasLifetime[selectedEntity] = false;
                        }
                    }
                }

                // Adding Mesh Components: { Cube, Sphere, Cylinder, Pyramid, Cone }
                if (registry.hasMesh[selectedEntity]) {
                    MeshComponent& mc = registry.meshes[selectedEntity];
                    
                    const char* shapes[] = { "Cube", "Sphere", "Cylinder", "Capsule", "Pyramid" };
                    int current = (int)mc.type;

                    if (ImGui::Combo("Shape", &current, shapes, IM_ARRAYSIZE(shapes))) {
                        mc.type = (MeshType)current;                                              
                        
                        // 1. Create the new mesh
                        mc.mesh = MeshManager::CreateMeshFromType(mc.type); 
                        
                        // 2. IMPORTANT: Update the Renderable component so the renderer picks up the change
                        if (registry.hasRenderable[selectedEntity]) {
                            registry.renderables[selectedEntity].mesh = mc.mesh;
                        } else {
                            // If it didn't have a renderable, add one
                            registry.AddRenderable(selectedEntity, { mc.mesh });
                        }
                        
                    }
                    if (ImGui::Button("Remove Mesh")) {
                        registry.hasMesh[selectedEntity] = false;
                    }
            
                }
                
                // Adding PlayerControllerComponent 
                if (registry.hasPlayerController[selectedEntity]) {
                    ImGui::Separator();
                    ImGui::Text("Player Controller Settings");

                    ImGui::DragFloat("Move Speed", &registry.playerControllers[selectedEntity].moveSpeed, 0.1f);
                    
                    // --- KEY BINDINGS UI ---
                    ImGui::Text("Key Bindings");
                    
                    // Helper lambda to make the code cleaner for 4 keys
                    auto RebindButton = [&](const char* label, int* key) {
                        ImGui::Text("%s: %s", label, SDL_GetKeyName(*key));
                        ImGui::SameLine();
                        if (ImGui::Button(("Rebind##" + std::string(label)).c_str())) {
                            keyToRebind = key;
                            isWaitingForKey = true;
                        }
                    };

                    RebindButton("Up",    &registry.playerControllers[selectedEntity].keyUp);
                    RebindButton("Down",  &registry.playerControllers[selectedEntity].keyDown);
                    RebindButton("Left",  &registry.playerControllers[selectedEntity].keyLeft);
                    RebindButton("Right", &registry.playerControllers[selectedEntity].keyRight);

                    // --- MOUSE OPTIONS ---
                    ImGui::Separator();
                    ImGui::Text("Mouse Settings");
                    ImGui::Checkbox("Use Mouse Look", &registry.playerControllers[selectedEntity].useMouseLook);
                    
                    // If enabled, allow them to choose the button
                    if (registry.playerControllers[selectedEntity].useMouseLook) {
                        const char* mouseButtons[] = { "Left", "Middle", "Right", "X1", "X2" };
                        int currentButton = registry.playerControllers[selectedEntity].mouseLookButton - 1; // SDL buttons are 1-based
                        if (ImGui::Combo("Look Button", &currentButton, mouseButtons, IM_ARRAYSIZE(mouseButtons))) {
                            registry.playerControllers[selectedEntity].mouseLookButton = currentButton + 1;
                        }
                    }

                    // Game Mode Option for First Person View or Thrid Person View
                    if (selectedEntity != (Entity)-1 && registry.hasPlayerController[selectedEntity]) {
                        PlayerControllerComponent& ctrl = registry.playerControllers[selectedEntity];

                        if (ImGui::CollapsingHeader("Player Controller", ImGuiTreeNodeFlags_DefaultOpen)) {
                            // Define the items for the dropdown
                            const char* viewModes[] = { "First Person (FPS)", "Third Person" };
                            
                            // Map the boolean to an integer index (0 = FPS, 1 = 3rd Person)
                            int currentItem = ctrl.isThirdPerson ? 1 : 0;
                            
                            if (ImGui::Combo("Camera View", &currentItem, viewModes, IM_ARRAYSIZE(viewModes))) {
                                // Update the boolean based on the selection
                                ctrl.isThirdPerson = (currentItem == 1);
                                Logger::Log(ctrl.isThirdPerson ? "Switched to Third Person" : "Switched to FPS");
                            }

                            if (ImGui::CollapsingHeader("Player Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::DragFloat3("3rd Person Offset", &ctrl.thirdPersonOffset.x, 0.1f);
                                ImGui::DragFloat3("FPS Offset", &ctrl.fpsOffset.x, 0.1f);
                                }
                        }
                        
                    }

                    if (ImGui::Button("Remove Player Controller")) {
                        registry.hasPlayerController[selectedEntity] = false;
                    }
                }

                   
            }    
                  

        ImGui::End();

        
        ImGui::Begin("Output Console");
			float buttonWidth = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float availableWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);

			if (ImGui::Button("Clear")) {
				Logger::ClearLogs();
			}

			ImGui::Columns(2, "ConsoleColumns", true);
            ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.85f);

            const std::vector<std::string>& messages = Logger::GetLogMessages();
            // Sent all logMessages here
            for (std::vector<std::string>::const_iterator it = messages.begin(); it != messages.end(); ++it) {
                ImGui::TextUnformatted(it->c_str());
            }
            // with Ranged-Based for loop
            // for (const auto& msg : Logger::GetLogMessages()){
            //     ImGui::TextUnformatted(msg.c_str());
            // }        
            

            // Move to the right column
            ImGui::NextColumn();
            if (ImGui::CollapsingHeader("Camera Info")) {
                // Displaying the vector components
                ImGui::Text("Position X: %.2f", editorCamera.position.x);
                ImGui::Text("Position Y: %.2f", editorCamera.position.y);
                ImGui::Text("Position Z: %.2f", editorCamera.position.z);

                ImGui::Text("Pitch: %.2f", editorCamera.pitch);
                ImGui::Text("Yaw: %.2f", editorCamera.yaw);
            }
            if (ImGui::Button("Origin")) {
                requestCameraReset = true;
            }
        ImGui::Columns(1);     
        ImGui::End();        
                       
        glDrawArrays(GL_LINES, 0, m_gridCount);
    }


    

    void End() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());  
    }

    void Draw(Camera& camera, Shader& shader, float aspect) {       
        // 1. Setup Matrices
        glm::mat4 viewProj = camera.GetProjectionMatrix(aspect) * camera.GetViewMatrix();
      
        // 2. Draw Grid
        shader.use();
        shader.setMat4("u_ViewProjection", viewProj);
        
        glBindVertexArray(m_gridVAO);
        glDrawArrays(GL_LINES, 0, m_gridCount);
        glBindVertexArray(0);
    }

};
