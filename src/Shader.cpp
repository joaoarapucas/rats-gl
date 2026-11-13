#include "Shader.h"

#include <cstdio>
#include <fstream>
#include <sstream>

Shader::~Shader()
{
    Clear();
}

bool Shader::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
    return Compile(vertexCode, fragmentCode);
}

bool Shader::CreateFromFile(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode, fragmentCode;
    if (!ReadFile(vertexPath, vertexCode) || !ReadFile(fragmentPath, fragmentCode)) {
        return false;
    }
    return Compile(vertexCode.c_str(), fragmentCode.c_str());
}

bool Shader::ReadFile(const char* path, std::string& out)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "Shader: arquivo nao encontrado (%s)\n", path);
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

GLuint Shader::CompileStage(GLenum type, const char* code)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "Shader: erro de compilacao (%s)\n%s\n",
                     type == GL_VERTEX_SHADER ? "vertex" : "fragment", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::Compile(const char* vertexCode, const char* fragmentCode)
{
    Clear();

    GLuint vShader = CompileStage(GL_VERTEX_SHADER, vertexCode);
    GLuint fShader = CompileStage(GL_FRAGMENT_SHADER, fragmentCode);
    if (!vShader || !fShader) {
        if (vShader) glDeleteShader(vShader);
        if (fShader) glDeleteShader(fShader);
        return false;
    }

    programId = glCreateProgram();
    glAttachShader(programId, vShader);
    glAttachShader(programId, fShader);
    glLinkProgram(programId);

    // Após o link os objetos de shader não são mais necessários
    glDetachShader(programId, vShader);
    glDetachShader(programId, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    GLint success = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(programId, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "Shader: erro de link\n%s\n", infoLog);
        Clear();
        return false;
    }

    uniformModel = glGetUniformLocation(programId, "model");
    uniformView = glGetUniformLocation(programId, "view");
    uniformProjection = glGetUniformLocation(programId, "projection");
    uniformTexture = glGetUniformLocation(programId, "theTexture");
    return true;
}

void Shader::Use() const
{
    glUseProgram(programId);
}

void Shader::Clear()
{
    if (programId) glDeleteProgram(programId);
    programId = 0;
    uniformModel = uniformView = uniformProjection = uniformTexture = -1;
}
