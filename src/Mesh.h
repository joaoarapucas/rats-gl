#pragma once

#include <cstddef>

#include <glad/glad.h>

// Malha indexada. Layout de vértice: posição (x, y, z) + coordenada de textura (u, v).
class Mesh
{
public:
    static constexpr int FLOATS_PER_VERTEX = 5;

    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // vertexCount: número de vértices (não de floats); indexCount: número de índices
    void Create(const GLfloat* vertices, std::size_t vertexCount,
                const GLuint* indices, std::size_t indexCount);
    void Render() const;
    void Clear();

private:
    GLuint VAO = 0, VBO = 0, IBO = 0;
    GLsizei indexCount = 0;
};
