#pragma once

#include <memory>
#include "core/Engine.h"
#include "core/Camera.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_sdl3.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <SDL3/SDL.h>


class EditorLayer {
private: 
    std::vector<float> gridVertices;
    unsigned int m_gridVAO;
    unsigned int m_gridVBO;
    int m_gridCount; // grid count
    int m_width = 0, m_height = 0;

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
    ~EditorLayer() {
        glDeleteVertexArrays(1, &m_gridVAO);
        glDeleteBuffers(1, &m_gridVBO);
    }

    void Init(SDL_Window* window, SDL_GLContext context) {
        ImGui::CreateContext();
    
        // Enable the Docking Feature
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = 1.6f;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 410");                

        SDL_GetWindowSizeInPixels(window, &m_width, &m_height);
        glViewport(0, 0, m_width, m_height);
        m_gridCount = SetupGrid(m_gridVAO, m_gridVBO, m_width);        

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
        
        // TEmp
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