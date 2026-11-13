#include "MazeGeometry.h"

#include <cmath>

namespace rats {

bool MazeLayout::IsValid() const
{
    return width > 0 && height > 0 &&
           cells.size() == static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

bool MazeLayout::HasWall(int x, int y, Side side) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return false;

    static const int dx[] = {0, 1, 0, -1};
    static const int dy[] = {-1, 0, 1, 0};

    if (cells[static_cast<std::size_t>(y) * width + x][side]) return true;

    const int nx = x + dx[side];
    const int ny = y + dy[side];
    if (nx < 0 || nx >= width || ny < 0 || ny >= height) return false;

    const Side opposite = static_cast<Side>((side + 2) % 4);
    return cells[static_cast<std::size_t>(ny) * width + nx][opposite];
}

namespace {

struct Vertex
{
    float x, y, z, u, v;
};

void PushVertex(std::vector<float>& out, const Vertex& v)
{
    out.insert(out.end(), {v.x, v.y, v.z, v.u, v.v});
}

// Quad a-b-c-d (em ordem ao redor do perímetro) como dois triângulos.
void PushQuad(std::vector<float>& out, const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d)
{
    PushVertex(out, a);
    PushVertex(out, b);
    PushVertex(out, c);
    PushVertex(out, c);
    PushVertex(out, d);
    PushVertex(out, a);
}

// Parede vertical de (x0, z0) a (x1, z1), do chão até 'height'.
void PushWall(std::vector<float>& out, float x0, float z0, float x1, float z1,
              float height, float vMax)
{
    PushQuad(out,
             {x0, 0.0f,   z0, 0.0f, 0.0f},
             {x1, 0.0f,   z1, 1.0f, 0.0f},
             {x1, height, z1, 1.0f, vMax},
             {x0, height, z0, 0.0f, vMax});
}

// Plano horizontal cobrindo todo o labirinto na altura y.
void PushPlane(std::vector<float>& out, float y, float sizeX, float sizeZ, float uMax, float vMax)
{
    PushQuad(out,
             {0.0f,  y, 0.0f,  0.0f, 0.0f},
             {sizeX, y, 0.0f,  uMax, 0.0f},
             {sizeX, y, sizeZ, uMax, vMax},
             {0.0f,  y, sizeZ, 0.0f, vMax});
}

int CurrentVertex(const std::vector<float>& v)
{
    return static_cast<int>(v.size()) / MazeGeometry::FLOATS_PER_VERTEX;
}

} // namespace

MazeGeometry GenerateMazeGeometry(const MazeLayout& layout, float cellSize, float wallHeight)
{
    MazeGeometry geo;
    if (!layout.IsValid() || !std::isfinite(cellSize) || !std::isfinite(wallHeight) ||
        cellSize <= 0.0f || wallHeight <= 0.0f) {
        return geo;
    }

    const int w = layout.width;
    const int h = layout.height;
    const float sizeX = w * cellSize;
    const float sizeZ = h * cellSize;
    const float wallV = wallHeight / cellSize;

    // Pior caso: todas as paredes presentes -> w*(h+1) + h*(w+1) paredes
    const std::size_t maxWalls = static_cast<std::size_t>(w) * (h + 1) + static_cast<std::size_t>(h) * (w + 1);
    geo.vertices.reserve((2 + maxWalls) * 6 * MazeGeometry::FLOATS_PER_VERTEX);

    geo.floor.first = CurrentVertex(geo.vertices);
    PushPlane(geo.vertices, 0.0f, sizeX, sizeZ, static_cast<float>(w), static_cast<float>(h));
    geo.floor.count = CurrentVertex(geo.vertices) - geo.floor.first;

    geo.ceiling.first = CurrentVertex(geo.vertices);
    PushPlane(geo.vertices, wallHeight, sizeX, sizeZ, static_cast<float>(w), static_cast<float>(h));
    geo.ceiling.count = CurrentVertex(geo.vertices) - geo.ceiling.first;

    // Cada aresta da grade é emitida uma única vez: cada célula emite suas
    // paredes Top e Left; a última linha emite Bottom e a última coluna, Right.
    geo.walls.first = CurrentVertex(geo.vertices);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float x0 = x * cellSize, x1 = (x + 1) * cellSize;
            const float z0 = y * cellSize, z1 = (y + 1) * cellSize;

            if (layout.HasWall(x, y, Top)) PushWall(geo.vertices, x0, z0, x1, z0, wallHeight, wallV);
            if (layout.HasWall(x, y, Left)) PushWall(geo.vertices, x0, z0, x0, z1, wallHeight, wallV);
            if (y == h - 1 && layout.HasWall(x, y, Bottom)) PushWall(geo.vertices, x0, z1, x1, z1, wallHeight, wallV);
            if (x == w - 1 && layout.HasWall(x, y, Right)) PushWall(geo.vertices, x1, z0, x1, z1, wallHeight, wallV);
        }
    }
    geo.walls.count = CurrentVertex(geo.vertices) - geo.walls.first;

    return geo;
}

} // namespace rats
