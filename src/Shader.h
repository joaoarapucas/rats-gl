#pragma once

#include <string>

#include <glad/glad.h>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool CreateFromString(const char* vertexCode, const char* fragmentCode);
    bool CreateFromFile(const char* vertexPath, const char* fragmentPath);
    void Use() const;
    void Clear();

    GLint GetUniformModel() const { return uniformModel; }
    GLint GetUniformView() const { return uniformView; }
    GLint GetUniformProjection() const { return uniformProjection; }
    GLint GetUniformTexture() const { return uniformTexture; }

private:
    GLuint programId = 0;
    GLint uniformModel = -1, uniformView = -1, uniformProjection = -1, uniformTexture = -1;

    static bool ReadFile(const char* path, std::string& out);
    static GLuint CompileStage(GLenum type, const char* code);
    bool Compile(const char* vertexCode, const char* fragmentCode);
};
