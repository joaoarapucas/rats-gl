#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(int width = 800, int height = 600, const char* title = "rats");
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Cria a janela e o contexto OpenGL e carrega as funções via GLAD.
    // Retorna false em caso de falha.
    bool Initialize();

    bool ShouldClose() const { return glfwWindowShouldClose(window); }
    void SwapBuffers() { glfwSwapBuffers(window); }

    int GetBufferWidth() const { return bufferWidth; }
    int GetBufferHeight() const { return bufferHeight; }
    float GetAspectRatio() const;

    const bool* GetKeys() const { return keys; }
    float GetXChange();
    float GetYChange();

private:
    static constexpr int KEY_COUNT = 1024;

    GLFWwindow* window = nullptr;
    int width, height;
    const char* title;
    int bufferWidth = 0, bufferHeight = 0;
    bool glfwInitialized = false;

    bool keys[KEY_COUNT] = {};

    bool mouseFirstMove = true;
    float lastX = 0.0f, lastY = 0.0f;
    float xChange = 0.0f, yChange = 0.0f;

    void CreateCallbacks();
    static void HandleKeys(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void HandleMouse(GLFWwindow* window, double xPos, double yPos);
    static void HandleFramebufferSize(GLFWwindow* window, int width, int height);
};
