#include "Mesh.h"

Mesh::~Mesh()
{
    Clear();
}

void Mesh::Create(const GLfloat* vertices, std::size_t vertexCount,
                  const GLuint* indices, std::size_t indexCount)
{
    Clear();
    this->indexCount = static_cast<GLsizei>(indexCount);

    const GLsizei stride = FLOATS_PER_VERTEX * sizeof(GLfloat);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, vertices, GL_STATIC_DRAW);

    // O binding do IBO fica gravado no VAO
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // location 0: posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    // location 1: coordenada de textura
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Desvincula o VAO antes do IBO, senão o IBO seria removido do VAO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::Render() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::Clear()
{
    if (IBO) glDeleteBuffers(1, &IBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    IBO = VBO = VAO = 0;
    indexCount = 0;
}
