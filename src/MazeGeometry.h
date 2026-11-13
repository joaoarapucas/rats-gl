#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace rats {

// Lados de uma célula. A ordem (Top, Right, Bottom, Left) é a mesma usada pelo
// gerador de labirintos, então os índices podem ser copiados diretamente.
enum Side { Top = 0, Right = 1, Bottom = 2, Left = 3 };

// Descrição de um labirinto independente de qualquer gerador: grade
// width x height, armazenada por linhas (índice = y * width + x).
// Convenção: Top aponta para y - 1 e Left para x - 1.
struct MazeLayout
{
    using CellWalls = std::array<bool, 4>;

    int width = 0;
    int height = 0;
    std::vector<CellWalls> cells;

    bool IsValid() const;

    // Uma parede entre duas células existe se qualquer um dos lados a declarar.
    bool HasWall(int x, int y, Side side) const;
};

// Converte qualquer grade que exponha a interface
//   int get_width() const; int get_height() const;
//   cell(x, y).walls[Side]   (Top, Right, Bottom, Left)
// para MazeLayout, sem depender do tipo concreto do gerador.
template <typename Grid>
MazeLayout ToMazeLayout(const Grid& grid)
{
    MazeLayout layout;
    layout.width = grid.get_width();
    layout.height = grid.get_height();
    if (layout.width <= 0 || layout.height <= 0) return MazeLayout{};

    layout.cells.resize(static_cast<std::size_t>(layout.width) * static_cast<std::size_t>(layout.height));
    for (int y = 0; y < layout.height; ++y) {
        for (int x = 0; x < layout.width; ++x) {
            const auto& cell = grid.cell(x, y);
            auto& out = layout.cells[static_cast<std::size_t>(y) * layout.width + x];
            for (int side = 0; side < 4; ++side) out[side] = static_cast<bool>(cell.walls[side]);
        }
    }
    return layout;
}

// Faixa de vértices (para glDrawArrays) dentro do buffer gerado.
struct VertexRange
{
    int first = 0;
    int count = 0;
};

// Geometria do labirinto em um único buffer de triângulos não indexados.
// Layout de vértice: x, y, z, u, v.
//
// Espaço do modelo: a célula (x, y) ocupa [x, x+1] * cellSize em X e
// [y, y+1] * cellSize em Z; o chão fica em Y = 0 e o teto em Y = wallHeight.
// As coordenadas de textura repetem a textura uma vez por célula; nas paredes
// V vai até wallHeight / cellSize, preservando a proporção da textura.
struct MazeGeometry
{
    static constexpr int FLOATS_PER_VERTEX = 5;

    std::vector<float> vertices;
    VertexRange floor;
    VertexRange ceiling;
    VertexRange walls;

    int VertexCount() const { return static_cast<int>(vertices.size()) / FLOATS_PER_VERTEX; }
};

// Retorna geometria vazia se o layout for inválido ou as dimensões não forem positivas.
MazeGeometry GenerateMazeGeometry(const MazeLayout& layout, float cellSize, float wallHeight);

} // namespace rats
