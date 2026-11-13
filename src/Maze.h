#pragma once

#include <memory>
#include <string>

#include <amazing/maze.h>
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
//   maze.SetGenerator(amazing::GenAlgo::Prim);  // tipo de labirinto
//   maze.Generate(15, 10);                      // gera com o amazing e constrói
//   // ou: maze.Build(rats::ToMazeLayout(grade)); // qualquer grade compatível
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

    // Algoritmo (tipo de labirinto) usado por Generate. Padrão: DFS.
    void SetGenerator(amazing::GenAlgo algo) { generator = algo; }
    amazing::GenAlgo GetGenerator() const { return generator; }

    // Gera um labirinto width x height com o algoritmo escolhido e constrói o
    // VBO. Retorna false (sem alterar nada) se as dimensões não forem positivas.
    bool Generate(int width, int height);

    // Grade do amazing criada pelo último Generate, usada para resolver o
    // labirinto (ex.: rats::Rat::Solve). nullptr se o labirinto atual veio de
    // Build ou se nada foi gerado.
    const amazing::Maze* GetGrid() const { return grid.get(); }

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
    // Mesmo que CellCenter, para posições fracionárias da grade (ex.: um
    // personagem entre duas células): (0, 0) é o centro da célula (0, 0).
    glm::vec3 GridToWorld(glm::vec2 gridPosition) const;

private:
    static bool LoadTexture(const std::string& path, std::unique_ptr<Texture>& slot);
    static bool IsValidDimension(float value);
    void Upload();
    void DrawRange(const VertexRange& range, const Texture* texture) const;

    float cellSize;
    float wallHeight;
    MazeLayout layout;
    amazing::GenAlgo generator = amazing::GenAlgo::DFS;
    std::unique_ptr<amazing::Maze> grid;

    std::unique_ptr<Texture> wallTexture;
    std::unique_ptr<Texture> floorTexture;
    std::unique_ptr<Texture> ceilingTexture;

    GLuint VAO = 0, VBO = 0;
    VertexRange floorRange, ceilingRange, wallRange;
};

} // namespace rats
