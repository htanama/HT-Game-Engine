#pragma once
#include "ECS.h"
#include "Shader.h"

void RenderSystem(Registry& reg, Shader& shader, float time) {
    for (Entity i = 0; i < reg.hasTransform.size(); ++i) {
        if (reg.hasTransform[i] && reg.hasRenderable[i]) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, reg.transforms[i].position);
            
            // Add the same rotation logic you had in your main loop
            model = glm::rotate(model, time * glm::radians(50.0f), glm::vec3(0.5f, -1.0f, 0.0f));
            
            shader.setMat4("model", model);
            reg.renderables[i].mesh->draw();
        }
    }
}