// include/editor/EditorLayer.h
#pragma once

#include <memory>
#include "../core/Engine.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include "../SDL3-3.4.8/include/SDL3/SDL.h"

class EditorLayer {
private:
    std::shared_ptr<Renderer> m_Renderer; 
    
public:
    void Init(SDL_Window* window, SDL_GLContext context) {
        ImGui::CreateContext();
    
        // Enable the Docking Feature
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.6f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void Begin() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void Draw() 
    {
        // Dynamically get the size of the current UI window, not the whole application
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // 1. GLOBAL MENU BAR (Must be outside the DockSpace Host)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene"))    { /* Logic */ }
                if (ImGui::MenuItem("Save Scene"))   { /* Logic */ }
                if (ImGui::MenuItem("Load Scene"))   { /* Logic */ }
                if (ImGui::MenuItem("Exit"))        { Engine::SetIsRunning(false); }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // Create a dockspace in main viewport, where central node is transparent.
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

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
            ImGui::EndChild();
            ImGui::BeginChild("FileChild", ImVec2(0, 0), true);
            ImGui::Text("File System");
            ImGui::EndChild();
        ImGui::End();

        ImGui::Begin("Inspector");
            ImGui::Text("Transform");
            static float pos[3], rot[3], scale[3] = {1.0f, 1.0f, 1.0f};
            ImGui::DragFloat3("Position", pos);
            ImGui::DragFloat3("Rotation", rot);
            ImGui::DragFloat3("Scale", scale);
        ImGui::End();

        ImGui::Begin("Output Console");
            const std::vector<std::string>& messages = Logger::GetLogMessages();
            // Sent all logMessages here
            for (std::vector<std::string>::const_iterator it = messages.begin(); it != messages.end(); ++it) {
                ImGui::TextUnformatted(it->c_str());
            }
            // with Ranged-Based for loop
            // for (const auto& msg : Logger::GetLogMessages()){
            //     ImGui::TextUnformatted(msg.c_str());
            // }
        ImGui::End();

    }

    void End() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }



};