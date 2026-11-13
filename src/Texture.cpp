#include "Texture.h"

#include <cstdio>
#include <utility>

#include "stb_image.h"

Texture::Texture(std::string path) : path(std::move(path))
{
}

Texture::~Texture()
{
    Clear();
}

bool Texture::Load()
{
    Clear();

    // OpenGL espera a origem (0,0) no canto inferior esquerdo
    stbi_set_flip_vertically_on_load(true);

    // Força 4 canais: a imagem sempre casa com GL_RGBA, qualquer que seja o arquivo
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        std::fprintf(stderr, "Texture: erro ao carregar %s (%s)\n", path.c_str(), stbi_failure_reason());
        return false;
    }

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

void Texture::Use(GLenum unit) const
{
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

void Texture::Clear()
{
    if (textureId) glDeleteTextures(1, &textureId);
    textureId = 0;
    width = height = channels = 0;
}
