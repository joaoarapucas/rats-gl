#pragma once

#include <memory>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "MazeGeometry.h"
#include "Texture.h"

namespace rats {

// Renderizador de labirinto. Toda a geometria (chão, teto e paredes) fica em
// um único VBO; cada superfície é desenhada com sua própria textura.
//
// Uso:
//   rats::Maze maze(2.0f, 3.0f);                // cellSize, wallHeight
//   maze.SetWallTexture("textures/brick.png");
//   maze.SetFloorTexture("textures/ground.png");
//   maze.Build(rats::ToMazeLayout(gerador));    // qualquer grade compatível
//   ...
//   maze.Render();                              // com o shader e a matriz model já definidos
//
// Superfícies sem textura configurada não são desenhadas (ex.: sem teto).
// Todos os métodos que tocam OpenGL exigem um contexto ativo.
class Maze
{
public:
    explicit Maze(float cellSize = 1.0f, float wallHeight = 1.0f);
    ~Maze();

    Maze(const Maze&) = delete;
    Maze& operator=(const Maze&) = delete;

    // Retornam false (sem alterar nada) se o valor não for positivo e finito.
    // Se o labirinto já foi construído, o VBO é regerado.
    bool SetCellSize(float size);
    bool SetWallHeight(float height);
    bool SetDimensions(float cellSize, float wallHeight);

    // Carregam a textura do arquivo. Retornam false se o carregamento falhar,
    // mantendo a textura anterior. Caminho vazio remove a textura (e a superfície).
    bool SetWallTexture(const std::string& path);
    bool SetFloorTexture(const std::string& path);
    bool SetCeilingTexture(const std::string& path);

    // Gera o VBO a partir do layout. Retorna false se o layout for inválido.
    bool Build(const MazeLayout& layout);

    void Render() const;
    void Clear();

    float GetCellSize() const { return cellSize; }
    float GetWallHeight() const { return wallHeight; }
    int GetWidth() const { return layout.width; }
    int GetHeight() const { return layout.height; }
    const MazeLayout& GetLayout() const { return layout; }

    // Centro da célula (x, y) no espaço do modelo, na altura do chão.
    glm::vec3 CellCenter(int x, int y) const;

private:
    static bool LoadTexture(const std::string& path, std::unique_ptr<Texture>& slot);
    static bool IsValidDimension(float value);
    void Upload();
    void DrawRange(const VertexRange& range, const Texture* texture) const;

    float cellSize;
    float wallHeight;
    MazeLayout layout;

    std::unique_ptr<Texture> wallTexture;
    std::unique_ptr<Texture> floorTexture;
    std::unique_ptr<Texture> ceilingTexture;

    GLuint VAO = 0, VBO = 0;
    VertexRange floorRange, ceilingRange, wallRange;
};

} // namespace rats
