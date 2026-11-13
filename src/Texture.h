#pragma once

#include <string>

#include <glad/glad.h>

class Texture
{
public:
    explicit Texture(std::string path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool Load();
    void Use(GLenum unit = GL_TEXTURE0) const;
    void Clear();

private:
    GLuint textureId = 0;
    int width = 0, height = 0, channels = 0;
    std::string path;
};
