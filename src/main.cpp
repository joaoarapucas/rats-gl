#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <amazing/maze.h>

#include "Camera.h"
#include "Maze.h"
#include "Rat.h"
#include "Shader.h"
#include "Window.h"

namespace {

const char* VERTEX_SHADER_PATH = "shaders/vertex.glsl";
const char* FRAGMENT_SHADER_PATH = "shaders/fragment.glsl";
const char* WALL_TEXTURE_PATH = "textures/brick.png";
const char* FLOOR_TEXTURE_PATH = "textures/ground.png";
const char* CEILING_TEXTURE_PATH = "textures/ground.png";

const float CELL_SIZE = 2.0f;
const float WALL_HEIGHT = 2.5f;

// Labirinto gerado pela biblioteca amazing (external/amazing).
const int MAZE_WIDTH = 8;
const int MAZE_HEIGHT = 6;
const amazing::GenAlgo MAZE_ALGORITHM = amazing::GenAlgo::DFS;

// Rato: resolve o labirinto de (0, 0) até (largura - 1, altura - 1) e a câmera
// percorre o caminho que ele fez.
const amazing::SolveAlgo RAT_SOLVER = amazing::SolveAlgo::LeftWallFollower;
const float RAT_MOVE_SPEED = 2.0f;   // células por segundo
const float RAT_TURN_SPEED = 180.0f; // graus por segundo

// false: ignora o rato e libera a câmera (WASD + mouse).
const bool FOLLOW_RAT = true;

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

    Shader shader;
    if (!shader.CreateFromFile(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH)) return EXIT_FAILURE;

    rats::Maze maze(CELL_SIZE, WALL_HEIGHT);
    if (!maze.SetWallTexture(WALL_TEXTURE_PATH) ||
        !maze.SetFloorTexture(FLOOR_TEXTURE_PATH) ||
        !maze.SetCeilingTexture(CEILING_TEXTURE_PATH)) {
        return EXIT_FAILURE;
    }

    maze.SetGenerator(MAZE_ALGORITHM);
    if (!maze.Generate(MAZE_WIDTH, MAZE_HEIGHT)) {
        std::fprintf(stderr, "Maze: dimensoes invalidas\n");
        return EXIT_FAILURE;
    }

    rats::Rat rat(RAT_SOLVER);
    if (!rat.SetMoveSpeed(RAT_MOVE_SPEED) || !rat.SetTurnSpeed(RAT_TURN_SPEED)) {
        std::fprintf(stderr, "Rat: velocidade invalida\n");
        return EXIT_FAILURE;
    }
    if (!rat.Solve(*maze.GetGrid())) {
        std::fprintf(stderr, "Rat: o solver nao encontrou a saida\n");
    }

    // Câmera na altura dos olhos, no centro da célula inicial
    const float eyeHeight = WALL_HEIGHT * 0.5f;
    auto ratEye = [&]() {
        glm::vec3 eye = maze.GridToWorld(rat.GetPosition());
        eye.y = eyeHeight;
        return eye;
    };
    Camera camera(ratEye(), glm::vec3(0.0f, 1.0f, 0.0f), rat.GetYaw(), 0.0f, 3.0f, 0.1f);

    shader.Use();
    glUniform1i(shader.GetUniformTexture(), 0); // sampler -> GL_TEXTURE0

    float lastTime = static_cast<float>(glfwGetTime());

    for (long frame = 0; !window.ShouldClose() && frame != maxFrames; ++frame) {
        float now = static_cast<float>(glfwGetTime());
        float deltaTime = now - lastTime;
        lastTime = now;

        glfwPollEvents();
        if (FOLLOW_RAT) {
            rat.Update(deltaTime);
            camera.SetPose(ratEye(), rat.GetYaw());
        } else {
            camera.KeyControl(window.GetKeys(), deltaTime);
            camera.MouseControl(window.GetXChange(), window.GetYChange());
        }

        glClearColor(1.0f, 0.75f, 0.79f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.Use();

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), window.GetAspectRatio(), 0.1f, 100.0f);
        glUniformMatrix4fv(shader.GetUniformProjection(), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(shader.GetUniformView(), 1, GL_FALSE, glm::value_ptr(camera.CalculateViewMatrix()));

        glm::mat4 model(1.0f);
        glUniformMatrix4fv(shader.GetUniformModel(), 1, GL_FALSE, glm::value_ptr(model));
        maze.Render();

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
