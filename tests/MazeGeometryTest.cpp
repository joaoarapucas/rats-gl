// Testes da geração de geometria do labirinto (não precisam de contexto OpenGL).
#include <cmath>
#include <cstdio>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "MazeGeometry.h"

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

bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

using rats::MazeGeometry;
using rats::MazeLayout;

// Grade com a mesma interface do gerador de labirintos.
struct FakeGrid
{
    struct Cell { bool walls[4] = {true, true, true, true}; };
    int w, h;
    std::vector<Cell> cells;
    FakeGrid(int w, int h) : w(w), h(h), cells(static_cast<std::size_t>(w > 0 && h > 0 ? w * h : 0)) {}
    int get_width() const { return w; }
    int get_height() const { return h; }
    Cell& cell(int x, int y) { return cells[y * w + x]; }
    const Cell& cell(int x, int y) const { return cells[y * w + x]; }

    void Open(int x, int y, rats::Side side)
    {
        static const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
        cell(x, y).walls[side] = false;
        cell(x + dx[side], y + dy[side]).walls[(side + 2) % 4] = false;
    }
};

MazeLayout Closed(int w, int h)
{
    return rats::ToMazeLayout(FakeGrid(w, h));
}

const float* VertexAt(const MazeGeometry& g, int i)
{
    return &g.vertices[static_cast<std::size_t>(i) * MazeGeometry::FLOATS_PER_VERTEX];
}

int WallCount(const MazeGeometry& g) { return g.walls.count / 6; }

// Segmento (x0, z0, x1, z1) no chão de cada parede, normalizado.
std::set<std::tuple<float, float, float, float>> WallSegments(const MazeGeometry& g)
{
    std::set<std::tuple<float, float, float, float>> out;
    for (int wIdx = 0; wIdx < WallCount(g); ++wIdx) {
        const float* a = VertexAt(g, g.walls.first + wIdx * 6);
        const float* b = VertexAt(g, g.walls.first + wIdx * 6 + 1);
        float x0 = a[0], z0 = a[2], x1 = b[0], z1 = b[2];
        if (std::tie(x1, z1) < std::tie(x0, z0)) { std::swap(x0, x1); std::swap(z0, z1); }
        out.insert({x0, z0, x1, z1});
    }
    return out;
}

void TestInvalidInput()
{
    CHECK(GenerateMazeGeometry(MazeLayout{}, 1.0f, 1.0f).vertices.empty());

    MazeLayout wrongSize = Closed(2, 2);
    wrongSize.cells.pop_back();
    CHECK(!wrongSize.IsValid());
    CHECK(GenerateMazeGeometry(wrongSize, 1.0f, 1.0f).vertices.empty());

    const MazeLayout ok = Closed(2, 2);
    CHECK(GenerateMazeGeometry(ok, 0.0f, 1.0f).vertices.empty());
    CHECK(GenerateMazeGeometry(ok, 1.0f, -1.0f).vertices.empty());
    CHECK(GenerateMazeGeometry(ok, NAN, 1.0f).vertices.empty());
    CHECK(GenerateMazeGeometry(ok, 1.0f, INFINITY).vertices.empty());

    CHECK(!rats::ToMazeLayout(FakeGrid(0, 3)).IsValid());
}

void TestToMazeLayout()
{
    FakeGrid grid(3, 2);
    grid.Open(0, 0, rats::Right);
    grid.Open(2, 0, rats::Bottom);
    const MazeLayout layout = rats::ToMazeLayout(grid);

    CHECK(layout.IsValid());
    CHECK(layout.width == 3 && layout.height == 2);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 3; ++x)
            for (int s = 0; s < 4; ++s)
                CHECK(layout.cells[y * 3 + x][s] == grid.cell(x, y).walls[s]);
}

void TestSingleClosedCell()
{
    const float cs = 2.0f, wh = 3.0f;
    const MazeGeometry g = GenerateMazeGeometry(Closed(1, 1), cs, wh);

    CHECK(g.floor.first == 0 && g.floor.count == 6);
    CHECK(g.ceiling.first == 6 && g.ceiling.count == 6);
    CHECK(g.walls.first == 12 && g.walls.count == 24);
    CHECK(g.VertexCount() == 36);
    CHECK(g.vertices.size() == 36u * MazeGeometry::FLOATS_PER_VERTEX);

    for (int i = 0; i < g.floor.count; ++i) CHECK(Near(VertexAt(g, g.floor.first + i)[1], 0.0f));
    for (int i = 0; i < g.ceiling.count; ++i) CHECK(Near(VertexAt(g, g.ceiling.first + i)[1], wh));

    const auto segs = WallSegments(g);
    CHECK(segs.size() == 4);
    CHECK(segs.count({0.0f, 0.0f, cs, 0.0f}));   // Top
    CHECK(segs.count({0.0f, cs, cs, cs}));       // Bottom
    CHECK(segs.count({0.0f, 0.0f, 0.0f, cs}));   // Left
    CHECK(segs.count({cs, 0.0f, cs, cs}));       // Right
}

void TestOpenCellHasNoWalls()
{
    MazeLayout layout = Closed(1, 1);
    layout.cells[0] = {false, false, false, false};
    const MazeGeometry g = GenerateMazeGeometry(layout, 1.0f, 1.0f);
    CHECK(g.walls.count == 0);
    CHECK(g.VertexCount() == 12);
}

void TestSharedWallEmittedOnce()
{
    // Parede declarada pelos dois lados: uma só parede entre as células.
    const MazeGeometry closed = GenerateMazeGeometry(Closed(2, 1), 1.0f, 1.0f);
    CHECK(WallCount(closed) == 7);
    CHECK(WallSegments(closed).size() == 7);

    // Parede declarada só por um lado ainda existe (regra "qualquer lado").
    MazeLayout oneSided = Closed(2, 1);
    oneSided.cells[1][rats::Left] = false;
    CHECK(oneSided.HasWall(1, 0, rats::Left));
    CHECK(WallCount(GenerateMazeGeometry(oneSided, 1.0f, 1.0f)) == 7);

    // Aberta pelos dois lados: some.
    oneSided.cells[0][rats::Right] = false;
    CHECK(!oneSided.HasWall(0, 0, rats::Right));
    CHECK(WallCount(GenerateMazeGeometry(oneSided, 1.0f, 1.0f)) == 6);
}

void TestPerfectMazeWallCount()
{
    // Labirinto perfeito em serpentina: w*h - 1 passagens abertas.
    const int w = 8, h = 6;
    FakeGrid grid(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x + 1 < w; ++x) grid.Open(x, y, rats::Right);
        if (y + 1 < h) grid.Open(y % 2 == 0 ? w - 1 : 0, y, rats::Bottom);
    }
    const MazeGeometry g = GenerateMazeGeometry(rats::ToMazeLayout(grid), 1.5f, 2.0f);

    const int totalEdges = w * (h + 1) + h * (w + 1);
    const int expected = totalEdges - (w * h - 1);
    CHECK(WallCount(g) == expected);
    CHECK(static_cast<int>(WallSegments(g).size()) == expected); // sem duplicatas
}

void TestBoundsAndTexCoords()
{
    const int w = 5, h = 3;
    const float cs = 1.5f, wh = 4.0f;
    const MazeGeometry g = GenerateMazeGeometry(Closed(w, h), cs, wh);

    CHECK(WallCount(g) == w * (h + 1) + h * (w + 1));

    float maxFloorU = 0, maxFloorV = 0;
    for (int i = 0; i < g.floor.count; ++i) {
        const float* v = VertexAt(g, g.floor.first + i);
        maxFloorU = std::fmax(maxFloorU, v[3]);
        maxFloorV = std::fmax(maxFloorV, v[4]);
    }
    CHECK(Near(maxFloorU, static_cast<float>(w)));
    CHECK(Near(maxFloorV, static_cast<float>(h)));

    for (int i = 0; i < g.VertexCount(); ++i) {
        const float* v = VertexAt(g, i);
        CHECK(v[0] >= 0.0f && v[0] <= w * cs + 1e-5f);
        CHECK(v[2] >= 0.0f && v[2] <= h * cs + 1e-5f);
        CHECK(Near(v[1], 0.0f) || Near(v[1], wh));
    }

    for (int i = 0; i < g.walls.count; ++i) {
        const float* v = VertexAt(g, g.walls.first + i);
        // V proporcional à altura (textura não esticada) e paredes nas linhas da grade
        CHECK(Near(v[4], 0.0f) || Near(v[4], wh / cs));
        CHECK(Near(v[1], 0.0f) == Near(v[4], 0.0f));
        CHECK(Near(std::fmod(v[0], cs), 0.0f) || Near(std::fmod(v[2], cs), 0.0f));
    }
}

} // namespace

int main()
{
    TestInvalidInput();
    TestToMazeLayout();
    TestSingleClosedCell();
    TestOpenCellHasNoWalls();
    TestSharedWallEmittedOnce();
    TestPerfectMazeWallCount();
    TestBoundsAndTexCoords();

    std::printf("%d verificacoes, %d falhas\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
