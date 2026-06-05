// include/editor/EditorLayer.h
#pragma once
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

#include "../SDL3-3.4.8/include/SDL3/SDL.h"

class EditorLayer {
public:
    void Init(SDL_Window* window, SDL_GLContext context) {
        ImGui::CreateContext();
    
        // Enable the Docking Feature
        ImGuiIO& io = ImGui::GetIO();
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
        // 1. GLOBAL MENU BAR (Must be outside the DockSpace Host)
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene"))    { /* Logic */ }
                if (ImGui::MenuItem("Save Scene"))   { /* Logic */ }
                if (ImGui::MenuItem("Load Scene"))   { /* Logic */ }
                if (ImGui::MenuItem("Exit"))         { /* Exit Logic */ }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // 2. Setup the Host Window for the DockSpace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(0.0f); // Make host invisible

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking | 
                                    ImGuiWindowFlags_NoTitleBar | 
                                    ImGuiWindowFlags_NoCollapse | 
                                    ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | 
                                    ImGuiWindowFlags_NoBringToFrontOnFocus | 
                                    ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("MainDockSpaceHost", nullptr, host_flags);
        ImGui::PopStyleVar(2);

        // 3. Submit the DockSpace
        ImGuiID dockspace_id = ImGui::GetID("MyMainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // 4. Windows (These will now dock into the host)
        ImGui::Begin("LeftPanel");
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
        ImGui::Text("Log: Engine initialized...");
        ImGui::End();

        // 5. Close the Host Window (Must be at the very end)
        ImGui::End();
    }

    void End() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }



};