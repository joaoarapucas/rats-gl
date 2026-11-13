#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"

namespace {

const char* VERTEX_SHADER_PATH = "shaders/vertex.glsl";
const char* FRAGMENT_SHADER_PATH = "shaders/fragment.glsl";
const char* BRICK_TEXTURE_PATH = "textures/brick.png";
const char* GROUND_TEXTURE_PATH = "textures/ground.png";

// Cubo unitário centrado na origem: 4 vértices por face (UVs independentes por face).
// Layout: x, y, z, u, v
const GLfloat CUBE_VERTICES[] = {
    // frente (+Z)
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f, 0.0f, 1.0f,
    // trás (-Z)
     0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
     0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
    // esquerda (-X)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
    // direita (+X)
     0.5f, -0.5f,  0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
     0.5f,  0.5f,  0.5f, 0.0f, 1.0f,
    // topo (+Y)
    -0.5f,  0.5f,  0.5f, 0.0f, 0.0f,
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f,
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f,
    // base (-Y)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
     0.5f, -0.5f,  0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, 0.0f, 1.0f,
};

const GLuint CUBE_INDICES[] = {
     0,  1,  2,   2,  3,  0,
     4,  5,  6,   6,  7,  4,
     8,  9, 10,  10, 11,  8,
    12, 13, 14,  14, 15, 12,
    16, 17, 18,  18, 19, 16,
    20, 21, 22,  22, 23, 20,
};

// Chão 20x20 no plano Y=0; UV vai até 10 para repetir a textura (GL_REPEAT)
const GLfloat FLOOR_VERTICES[] = {
    -10.0f, 0.0f, -10.0f,  0.0f,  0.0f,
    -10.0f, 0.0f,  10.0f, 10.0f,  0.0f,
     10.0f, 0.0f,  10.0f, 10.0f, 10.0f,
     10.0f, 0.0f, -10.0f,  0.0f, 10.0f,
};

const GLuint FLOOR_INDICES[] = {
    0, 1, 3,
    1, 2, 3,
};

template <typename T, std::size_t N>
constexpr std::size_t CountOf(const T (&)[N]) { return N; }

} // namespace

int main(int argc, char** argv)
{
    // --frames N: encerra após N frames (útil para testes automatizados)
    long maxFrames = -1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0) maxFrames = std::strtol(argv[i + 1], nullptr, 10);
    }

    // A janela é declarada primeiro para ser destruída por último:
    // todos os objetos OpenGL abaixo precisam do contexto vivo no destrutor.
    Window window(1024, 768, "rats");
    if (!window.Initialize()) return EXIT_FAILURE;

    Mesh cube;
    cube.Create(CUBE_VERTICES, CountOf(CUBE_VERTICES) / Mesh::FLOATS_PER_VERTEX,
                CUBE_INDICES, CountOf(CUBE_INDICES));

    Mesh ground;
    ground.Create(FLOOR_VERTICES, CountOf(FLOOR_VERTICES) / Mesh::FLOATS_PER_VERTEX,
                 FLOOR_INDICES, CountOf(FLOOR_INDICES));

    Shader shader;
    if (!shader.CreateFromFile(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH)) return EXIT_FAILURE;

    Texture brickTexture(BRICK_TEXTURE_PATH);
    Texture groundTexture(GROUND_TEXTURE_PATH);
    if (!brickTexture.Load() || !groundTexture.Load()) return EXIT_FAILURE;

    Camera camera(glm::vec3(0.0f, 0.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 3.0f, 0.1f);

    shader.Use();
    glUniform1i(shader.GetUniformTexture(), 0); // sampler -> GL_TEXTURE0

    float lastTime = static_cast<float>(glfwGetTime());
    float angle = 0.0f;

    for (long frame = 0; !window.ShouldClose() && frame != maxFrames; ++frame) {
        float now = static_cast<float>(glfwGetTime());
        float deltaTime = now - lastTime;
        lastTime = now;

        glfwPollEvents();
        camera.KeyControl(window.GetKeys(), deltaTime);
        camera.MouseControl(window.GetXChange(), window.GetYChange());

        glClearColor(1.0f, 0.75f, 0.79f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();

        // Projeção e câmera antes dos draw calls
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), window.GetAspectRatio(), 0.1f, 100.0f);
        glUniformMatrix4fv(shader.GetUniformProjection(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(shader.GetUniformView(), 1, GL_FALSE, glm::value_ptr(camera.CalculateViewMatrix()));

        // Cubo girando sobre o eixo Y (45 graus/segundo)
        angle += 45.0f * deltaTime;
        if (angle >= 360.0f) angle -= 360.0f;

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(shader.GetUniformModel(), 1, GL_FALSE, glm::value_ptr(model));
        brickTexture.Use();
        cube.Render();

        // Chão
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(shader.GetUniformModel(), 1, GL_FALSE, glm::value_ptr(model));
        groundTexture.Use();
        ground.Render();

        glUseProgram(0);
        window.SwapBuffers();
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::fprintf(stderr, "OpenGL: erro 0x%04X\n", err);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
