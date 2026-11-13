// Testes do rats::Rat (não precisam de contexto OpenGL).
#include <cmath>
#include <cstdio>
#include <vector>

#include <amazing/maze.h>

#include "Rat.h"

namespace {

int failures = 0;
int checks = 0;

#define CHECK(cond)                                                                 \
    do {                                                                            \
        ++checks;                                                                   \
        if (!(cond)) {                                                              \
            ++failures;                                                             \
            std::fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                           \
    } while (0)

bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
bool Near(glm::vec2 a, glm::vec2 b, float eps = 1e-4f) { return Near(a.x, b.x, eps) && Near(a.y, b.y, eps); }

using amazing::Cell;
using amazing::GenAlgo;
using amazing::Move;
using amazing::SolveAlgo;
using rats::Rat;

const int DX[] = {0, 1, 0, -1};
const int DY[] = {-1, 0, 1, 0};

void Open(amazing::Maze& maze, int x, int y, int side)
{
    maze.cell(x, y).walls[side] = false;
    maze.cell(x + DX[side], y + DY[side]).walls[(side + 2) % 4] = false;
}

// Corredor em L:  (0,0) -> (1,0) -> (1,1)
amazing::Maze LMaze()
{
    amazing::Maze maze(2, 2);
    Open(maze, 0, 0, Cell::Right);
    Open(maze, 1, 0, Cell::Bottom);
    return maze;
}

void TestDefaultsAndSetters()
{
    Rat rat;
    CHECK(rat.GetSolver() == SolveAlgo::LeftWallFollower);
    CHECK(rat.IsFinished());
    CHECK(!rat.HasSolution());

    rat.SetSolver(SolveAlgo::RightWallFollower);
    CHECK(rat.GetSolver() == SolveAlgo::RightWallFollower);

    CHECK(rat.SetMoveSpeed(3.5f) && Near(rat.GetMoveSpeed(), 3.5f));
    CHECK(rat.SetTurnSpeed(90.0f) && Near(rat.GetTurnSpeed(), 90.0f));
    CHECK(!rat.SetMoveSpeed(0.0f));
    CHECK(!rat.SetMoveSpeed(-1.0f));
    CHECK(!rat.SetMoveSpeed(NAN));
    CHECK(!rat.SetTurnSpeed(INFINITY));
    CHECK(Near(rat.GetMoveSpeed(), 3.5f) && Near(rat.GetTurnSpeed(), 90.0f));

    // Sem caminho, Update não faz nada
    rat.Update(1.0f);
    CHECK(Near(rat.GetPosition(), glm::vec2(0.0f)));
}

void TestYawConvention()
{
    // Deve casar com Camera: front = (cos yaw, sin yaw) no plano XZ, e a grade
    // usa X = x e Z = y.
    for (int m = 0; m < 4; ++m) {
        const float yaw = glm::radians(Rat::YawOf(static_cast<Move>(m)));
        CHECK(Near(std::cos(yaw), static_cast<float>(DX[m])));
        CHECK(Near(std::sin(yaw), static_cast<float>(DY[m])));
    }
}

void TestSolveStartsFacingFirstMove()
{
    const amazing::Maze maze = LMaze();
    Rat rat;
    CHECK(rat.Solve(maze));
    CHECK(rat.HasSolution());
    CHECK(!rat.IsFinished());
    CHECK(rat.GetPath().size() == 2);
    CHECK(Near(rat.GetYaw(), 0.0f)); // primeiro passo: Right
    CHECK(Near(rat.GetPosition(), glm::vec2(0.0f, 0.0f)));
}

void TestMoveThenTurn()
{
    const amazing::Maze maze = LMaze();
    Rat rat;
    rat.SetMoveSpeed(1.0f);  // 1 célula/s
    rat.SetTurnSpeed(90.0f); // 90 graus/s
    CHECK(rat.Solve(maze));

    rat.Update(0.5f); // meio passo para a direita
    CHECK(Near(rat.GetPosition(), glm::vec2(0.5f, 0.0f)));
    CHECK(rat.GetCellX() == 0 && rat.GetStep() == 0);

    rat.Update(0.5f); // chega em (1, 0)
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 0.0f)));
    CHECK(rat.GetCellX() == 1 && rat.GetStep() == 1);
    CHECK(Near(rat.GetYaw(), 0.0f));

    rat.Update(0.5f); // vira 45 graus no lugar
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 0.0f)));
    CHECK(Near(rat.GetYaw(), 45.0f));

    rat.Update(0.5f); // termina a virada, ainda sem andar
    CHECK(Near(rat.GetYaw(), 90.0f));
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 0.0f)));

    rat.Update(0.25f);
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 0.25f)));

    rat.Update(10.0f); // passa do fim: para na saída
    CHECK(rat.IsFinished());
    CHECK(rat.GetCellX() == 1 && rat.GetCellY() == 1);
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 1.0f)));
    CHECK(Near(rat.GetYaw(), 90.0f));

    rat.Update(1.0f); // terminado: nada muda
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 1.0f)));
}

void TestLeftoverTimeCarriesOver()
{
    // Um único Update que cobre passo + virada + parte do passo seguinte.
    const amazing::Maze maze = LMaze();
    Rat rat;
    rat.SetMoveSpeed(2.0f);  // passo = 0.5 s
    rat.SetTurnSpeed(90.0f); // virada de 90 = 1 s
    CHECK(rat.Solve(maze));
    rat.Update(0.5f + 1.0f + 0.25f);
    CHECK(Near(rat.GetYaw(), 90.0f));
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 0.5f)));
}

void TestUTurnInDeadEnd()
{
    // (0,0) -- (1,0) -- (2,0)
    //            |
    //          (1,1)
    // A mão esquerda entra no beco (1,1) e volta: precisa virar 180 graus.
    amazing::Maze maze(3, 2);
    Open(maze, 0, 0, Cell::Right);
    Open(maze, 1, 0, Cell::Right);
    Open(maze, 1, 0, Cell::Bottom);

    Rat rat(SolveAlgo::RightWallFollower);
    CHECK(rat.Solve(maze, 0, 0, 2, 0));
    const amazing::Path& path = rat.GetPath();
    bool hasUTurn = false;
    for (std::size_t i = 1; i < path.size(); ++i)
        if ((static_cast<int>(path[i]) + 2) % 4 == static_cast<int>(path[i - 1])) hasUTurn = true;
    CHECK(hasUTurn);

    rat.SetMoveSpeed(1.0f);
    rat.SetTurnSpeed(90.0f);
    float maxYawChange = 0.0f;
    float lastYaw = rat.GetYaw();
    for (int i = 0; i < 1000 && !rat.IsFinished(); ++i) {
        rat.Update(0.01f);
        float d = std::fabs(rat.GetYaw() - lastYaw);
        if (d > 180.0f) d = 360.0f - d; // atravessou ±180
        if (d > maxYawChange) maxYawChange = d;
        lastYaw = rat.GetYaw();
    }
    CHECK(rat.IsFinished());
    CHECK(rat.GetCellX() == 2 && rat.GetCellY() == 0);
    // A virada é gradual: nunca mais que turnSpeed * dt por quadro.
    CHECK(maxYawChange <= 0.9f + 1e-3f);
}

// Simula com quadros pequenos e verifica que o rato nunca atravessa paredes,
// segue exatamente o caminho do solver e chega à saída.
void CheckWalk(const amazing::Maze& maze, SolveAlgo algo)
{
    Rat rat(algo);
    rat.SetMoveSpeed(7.0f);
    rat.SetTurnSpeed(500.0f);
    if (!rat.Solve(maze)) {
        CHECK(false);
        return;
    }

    const amazing::Path path = rat.GetPath();
    std::size_t lastStep = 0;
    int x = 0, y = 0;
    bool ok = true;
    for (int frame = 0; frame < 1000000 && !rat.IsFinished(); ++frame) {
        rat.Update(1.0f / 60.0f);

        // Avançou de célula: deve ser o próximo passo e por uma parede aberta
        while (lastStep < rat.GetStep()) {
            const int dir = static_cast<int>(path[lastStep]);
            if (maze.cell(x, y).walls[dir]) ok = false;
            x += DX[dir];
            y += DY[dir];
            ++lastStep;
        }
        if (rat.GetCellX() != x || rat.GetCellY() != y) ok = false;

        // Entre células, a posição fica no segmento do passo atual
        const glm::vec2 p = rat.GetPosition();
        if (!rat.IsFinished()) {
            const int dir = static_cast<int>(path[rat.GetStep()]);
            const glm::vec2 from(static_cast<float>(x), static_cast<float>(y));
            const glm::vec2 offset = p - from;
            const float along = offset.x * DX[dir] + offset.y * DY[dir];
            const float across = offset.x * DY[dir] - offset.y * DX[dir];
            if (along < -1e-4f || along > 1.0f + 1e-4f || std::fabs(across) > 1e-4f) ok = false;
        }
    }
    CHECK(ok);
    CHECK(rat.IsFinished());
    CHECK(rat.GetCellX() == maze.get_width() - 1 && rat.GetCellY() == maze.get_height() - 1);
}

void TestWalksGeneratedMazes()
{
    for (GenAlgo gen : {GenAlgo::DFS, GenAlgo::Prim, GenAlgo::BinaryTree}) {
        for (SolveAlgo solver : {SolveAlgo::LeftWallFollower, SolveAlgo::RightWallFollower}) {
            for (int run = 0; run < 10; ++run) {
                amazing::Maze maze(9 + run, 6 + run % 3);
                maze.generate(gen);
                CheckWalk(maze, solver);
            }
        }
    }
}

void TestFrameRateIndependence()
{
    amazing::Maze maze(12, 9);
    maze.generate(GenAlgo::DFS);

    Rat a, b;
    a.SetMoveSpeed(3.0f);
    b.SetMoveSpeed(3.0f);
    a.SetTurnSpeed(200.0f);
    b.SetTurnSpeed(200.0f);
    CHECK(a.Solve(maze) && b.Solve(maze));

    a.Update(4.0f);
    for (int i = 0; i < 400; ++i) b.Update(0.01f);
    CHECK(a.GetStep() == b.GetStep());
    CHECK(Near(a.GetPosition(), b.GetPosition(), 1e-2f));
    CHECK(Near(a.GetYaw(), b.GetYaw(), 1e-1f));
}

void TestUnsolvableAndRestart()
{
    amazing::Maze closed(3, 3); // todas as paredes fechadas
    Rat rat;
    CHECK(!rat.Solve(closed));
    CHECK(!rat.HasSolution());
    CHECK(rat.IsFinished());
    rat.Update(1.0f);
    CHECK(Near(rat.GetPosition(), glm::vec2(0.0f)));

    const amazing::Maze maze = LMaze();
    CHECK(rat.Solve(maze));
    rat.Update(100.0f);
    CHECK(rat.IsFinished());
    rat.Restart();
    CHECK(!rat.IsFinished() && rat.GetStep() == 0);
    CHECK(Near(rat.GetPosition(), glm::vec2(0.0f)) && Near(rat.GetYaw(), 0.0f));

    // Início == fim: caminho vazio, já terminado
    CHECK(rat.Solve(maze, 1, 1, 1, 1));
    CHECK(rat.HasSolution() && rat.IsFinished());
    CHECK(Near(rat.GetPosition(), glm::vec2(1.0f, 1.0f)));
}

void TestInvalidDeltaTime()
{
    const amazing::Maze maze = LMaze();
    Rat rat;
    CHECK(rat.Solve(maze));
    rat.Update(-1.0f);
    rat.Update(0.0f);
    rat.Update(NAN);
    rat.Update(INFINITY);
    CHECK(rat.GetStep() == 0);
    CHECK(Near(rat.GetPosition(), glm::vec2(0.0f)));
}

} // namespace

int main()
{
    TestDefaultsAndSetters();
    TestYawConvention();
    TestSolveStartsFacingFirstMove();
    TestMoveThenTurn();
    TestLeftoverTimeCarriesOver();
    TestUTurnInDeadEnd();
    TestWalksGeneratedMazes();
    TestFrameRateIndependence();
    TestUnsolvableAndRestart();
    TestInvalidDeltaTime();

    std::printf("%d verificacoes, %d falhas\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
