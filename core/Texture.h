#pragma once

#include <string>
#include <SDL3_image/SDL_image.h>

class Texture {
public:
  Texture::Texture(const std::string& path) : m_FilePath(path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        // Handle error: IMG_GetError()
        return;
    }

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    
    // Upload pixel data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    SDL_DestroySurface(surface); // Clean up CPU memory
}
  ~Texture();

  void Bind(unsigned int slot = 0) const;
  unsigned int GetID() const { return m_RendererID; }

private:
  unsigned int m_RendererID;
  std::string m_FilePath;
};
