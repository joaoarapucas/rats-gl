#pragma once

#include <cstddef>

#include <amazing/maze.h>
#include <amazing/solvers.h>
#include <glm/glm.hpp>

namespace rats {

// Personagem que resolve o labirinto e percorre o caminho encontrado.
//
// O rato anda de célula em célula: antes de cada passo ele vira (no próprio
// lugar) até ficar de frente para a próxima célula e só então anda até ela.
// O caminho é exatamente o que o solver percorreu, incluindo os retornos de
// becos sem saída.
//
// Uso:
//   rats::Rat rat(amazing::SolveAlgo::LeftWallFollower);
//   rat.SetMoveSpeed(2.0f);    // células por segundo
//   rat.SetTurnSpeed(180.0f);  // graus por segundo
//   if (rat.Solve(grid)) {
//       // a cada frame:
//       rat.Update(deltaTime);
//       glm::vec2 cell = rat.GetPosition();
//       float yaw = rat.GetYaw();
//   }
//
// Coordenadas: GetPosition() está no espaço da grade, com (x, y) inteiros no
// centro da célula (x, y). Direções seguem a convenção da Camera: yaw 0 aponta
// para +x (Right), 90 para +y (Down), 180 para -x (Left) e -90 para -y (Up).
class Rat
{
public:
    explicit Rat(amazing::SolveAlgo solver = amazing::SolveAlgo::LeftWallFollower);

    // Algoritmo usado pelo próximo Solve.
    void SetSolver(amazing::SolveAlgo algo) { solver = algo; }
    amazing::SolveAlgo GetSolver() const { return solver; }

    // Retornam false (sem alterar nada) se o valor não for positivo e finito.
    bool SetMoveSpeed(float cellsPerSecond);
    bool SetTurnSpeed(float degreesPerSecond);
    float GetMoveSpeed() const { return moveSpeed; }
    float GetTurnSpeed() const { return turnSpeed; }

    // Resolve o labirinto e posiciona o rato no início do caminho, já virado
    // para o primeiro passo. Sem argumentos de posição, vai de (0, 0) até
    // (largura - 1, altura - 1). Retorna false se o solver não encontrar
    // solução; nesse caso o rato fica parado no início e IsFinished() é true.
    bool Solve(const amazing::Maze& maze);
    bool Solve(const amazing::Maze& maze, int startX, int startY, int endX, int endY);

    // Avança a animação em deltaTime segundos. O tempo que sobra ao terminar
    // uma virada ou um passo é usado no movimento seguinte, então o resultado
    // não depende da taxa de quadros.
    void Update(float deltaTime);

    // Volta ao início do caminho atual.
    void Restart();

    bool IsFinished() const { return step >= path.size(); }
    bool HasSolution() const { return solved; }

    glm::vec2 GetPosition() const;
    float GetYaw() const { return yaw; }

    // Célula onde o rato está (ou de onde saiu, durante um passo).
    int GetCellX() const { return cellX; }
    int GetCellY() const { return cellY; }

    const amazing::Path& GetPath() const { return path; }
    // Índice do passo em andamento (== GetPath().size() ao terminar).
    std::size_t GetStep() const { return step; }

    // Yaw (graus) correspondente a um movimento.
    static float YawOf(amazing::Move move);

private:
    static bool IsValidSpeed(float value);

    amazing::SolveAlgo solver;
    float moveSpeed = 2.0f;
    float turnSpeed = 180.0f;

    amazing::Path path;
    bool solved = false;
    int startX = 0, startY = 0;

    int cellX = 0, cellY = 0;
    std::size_t step = 0;
    float progress = 0.0f; // fração [0, 1) do passo atual já percorrida
    float yaw = 0.0f;      // sempre em (-180, 180]
};

} // namespace rats
