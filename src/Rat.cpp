#include "Rat.h"

#include <cmath>

namespace rats {

namespace {

const int DX[] = {0, 1, 0, -1};
const int DY[] = {-1, 0, 1, 0};

// Normaliza um ângulo (graus) para (-180, 180].
float NormalizeAngle(float degrees)
{
    float a = std::fmod(degrees, 360.0f);
    if (a <= -180.0f) a += 360.0f;
    if (a > 180.0f) a -= 360.0f;
    return a;
}

} // namespace

Rat::Rat(amazing::SolveAlgo solver) : solver(solver) {}

bool Rat::IsValidSpeed(float value)
{
    return std::isfinite(value) && value > 0.0f;
}

bool Rat::SetMoveSpeed(float cellsPerSecond)
{
    if (!IsValidSpeed(cellsPerSecond)) return false;
    moveSpeed = cellsPerSecond;
    return true;
}

bool Rat::SetTurnSpeed(float degreesPerSecond)
{
    if (!IsValidSpeed(degreesPerSecond)) return false;
    turnSpeed = degreesPerSecond;
    return true;
}

float Rat::YawOf(amazing::Move move)
{
    switch (move) {
    case amazing::Move::Up: return -90.0f;
    case amazing::Move::Right: return 0.0f;
    case amazing::Move::Down: return 90.0f;
    case amazing::Move::Left: return 180.0f;
    }
    return 0.0f;
}

bool Rat::Solve(const amazing::Maze& maze)
{
    return Solve(maze, 0, 0, maze.get_width() - 1, maze.get_height() - 1);
}

bool Rat::Solve(const amazing::Maze& maze, int sx, int sy, int ex, int ey)
{
    auto result = maze.solve(solver, sx, sy, ex, ey);
    solved = result.has_value();
    path = solved ? std::move(*result) : amazing::Path{};
    startX = sx;
    startY = sy;
    Restart();
    return solved;
}

void Rat::Restart()
{
    cellX = startX;
    cellY = startY;
    step = 0;
    progress = 0.0f;
    yaw = path.empty() ? 0.0f : YawOf(path.front());
}

void Rat::Update(float deltaTime)
{
    if (!std::isfinite(deltaTime)) return;

    // Cada iteração termina uma virada ou um passo (ou consome todo o tempo),
    // então o laço executa no máximo 2 * path.size() vezes.
    while (deltaTime > 0.0f && !IsFinished()) {
        const int dir = static_cast<int>(path[step]);
        const float target = YawOf(path[step]);
        const float diff = NormalizeAngle(target - yaw);

        if (diff != 0.0f) {
            const float maxTurn = turnSpeed * deltaTime;
            if (std::fabs(diff) > maxTurn) {
                yaw = NormalizeAngle(yaw + (diff > 0.0f ? maxTurn : -maxTurn));
                return;
            }
            yaw = target;
            deltaTime -= std::fabs(diff) / turnSpeed;
            continue;
        }

        const float remaining = 1.0f - progress;
        const float maxMove = moveSpeed * deltaTime;
        if (maxMove < remaining) {
            progress += maxMove;
            return;
        }
        deltaTime -= remaining / moveSpeed;
        progress = 0.0f;
        cellX += DX[dir];
        cellY += DY[dir];
        ++step;
    }
}

glm::vec2 Rat::GetPosition() const
{
    glm::vec2 position(static_cast<float>(cellX), static_cast<float>(cellY));
    if (!IsFinished() && progress > 0.0f) {
        const int dir = static_cast<int>(path[step]);
        position += glm::vec2(DX[dir], DY[dir]) * progress;
    }
    return position;
}

} // namespace rats
