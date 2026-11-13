#include "Maze.h"

#include <cmath>
#include <utility>

namespace rats {

Maze::Maze(float cellSize, float wallHeight)
    : cellSize(IsValidDimension(cellSize) ? cellSize : 1.0f),
      wallHeight(IsValidDimension(wallHeight) ? wallHeight : 1.0f)
{
}

Maze::~Maze()
{
    Clear();
}

bool Maze::IsValidDimension(float value)
{
    return std::isfinite(value) && value > 0.0f;
}

bool Maze::SetCellSize(float size)
{
    return SetDimensions(size, wallHeight);
}

bool Maze::SetWallHeight(float height)
{
    return SetDimensions(cellSize, height);
}

bool Maze::SetDimensions(float newCellSize, float newWallHeight)
{
    if (!IsValidDimension(newCellSize) || !IsValidDimension(newWallHeight)) return false;
    cellSize = newCellSize;
    wallHeight = newWallHeight;
    if (VAO) Upload();
    return true;
}

bool Maze::LoadTexture(const std::string& path, std::unique_ptr<Texture>& slot)
{
    if (path.empty()) {
        slot.reset();
        return true;
    }
    auto texture = std::make_unique<Texture>(path);
    if (!texture->Load()) return false;
    slot = std::move(texture);
    return true;
}

bool Maze::SetWallTexture(const std::string& path)
{
    return LoadTexture(path, wallTexture);
}

bool Maze::SetFloorTexture(const std::string& path)
{
    return LoadTexture(path, floorTexture);
}

bool Maze::SetCeilingTexture(const std::string& path)
{
    return LoadTexture(path, ceilingTexture);
}

bool Maze::Generate(int width, int height)
{
    if (width <= 0 || height <= 0) return false;

    auto newGrid = std::make_unique<amazing::Maze>(width, height);
    newGrid->generate(generator);
    if (!Build(ToMazeLayout(*newGrid))) return false;
    grid = std::move(newGrid);
    return true;
}

bool Maze::Build(const MazeLayout& newLayout)
{
    if (!newLayout.IsValid()) return false;
    layout = newLayout;
    grid.reset(); // a grade antiga não corresponde mais ao layout
    Upload();
    return true;
}

void Maze::Upload()
{
    const MazeGeometry geo = GenerateMazeGeometry(layout, cellSize, wallHeight);
    const GLsizei stride = MazeGeometry::FLOATS_PER_VERTEX * sizeof(GLfloat);

    if (!VAO) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // location 0: posição
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
        // location 1: coordenada de textura
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(geo.vertices.size() * sizeof(GLfloat)),
                 geo.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    floorRange = geo.floor;
    ceilingRange = geo.ceiling;
    wallRange = geo.walls;
}

void Maze::DrawRange(const VertexRange& range, const Texture* texture) const
{
    if (!texture || range.count == 0) return;
    texture->Use();
    glDrawArrays(GL_TRIANGLES, range.first, range.count);
}

void Maze::Render() const
{
    if (!VAO) return;
    glBindVertexArray(VAO);
    DrawRange(floorRange, floorTexture.get());
    DrawRange(ceilingRange, ceilingTexture.get());
    DrawRange(wallRange, wallTexture.get());
    glBindVertexArray(0);
}

void Maze::Clear()
{
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    VBO = VAO = 0;
    floorRange = ceilingRange = wallRange = VertexRange{};
}

glm::vec3 Maze::CellCenter(int x, int y) const
{
    return GridToWorld(glm::vec2(static_cast<float>(x), static_cast<float>(y)));
}

glm::vec3 Maze::GridToWorld(glm::vec2 gridPosition) const
{
    return glm::vec3((gridPosition.x + 0.5f) * cellSize, 0.0f, (gridPosition.y + 0.5f) * cellSize);
}

} // namespace rats
