#include "Window.h"

#include <cstdio>

Window::Window(int width, int height, const char* title)
    : width(width), height(height), title(title)
{
}

Window::~Window()
{
    if (window) glfwDestroyWindow(window);
    if (glfwInitialized) glfwTerminate();
}

bool Window::Initialize()
{
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW: falha ao inicializar\n");
        return false;
    }
    glfwInitialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "GLFW: falha ao criar a janela\n");
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "GLAD: falha ao carregar as funcoes OpenGL\n");
        return false;
    }

    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
    glViewport(0, 0, bufferWidth, bufferHeight);
    glEnable(GL_DEPTH_TEST);

    glfwSwapInterval(1); // VSync
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetWindowUserPointer(window, this);
    CreateCallbacks();
    return true;
}

float Window::GetAspectRatio() const
{
    // Janela minimizada gera framebuffer 0x0
    if (bufferHeight == 0) return 1.0f;
    return static_cast<float>(bufferWidth) / static_cast<float>(bufferHeight);
}

float Window::GetXChange()
{
    float change = xChange;
    xChange = 0.0f;
    return change;
}

float Window::GetYChange()
{
    float change = yChange;
    yChange = 0.0f;
    return change;
}

void Window::CreateCallbacks()
{
    glfwSetKeyCallback(window, HandleKeys);
    glfwSetCursorPosCallback(window, HandleMouse);
    glfwSetFramebufferSizeCallback(window, HandleFramebufferSize);
}

void Window::HandleKeys(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key >= 0 && key < KEY_COUNT) {
        if (action == GLFW_PRESS) {
            self->keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            self->keys[key] = false;
        }
    }
}

void Window::HandleMouse(GLFWwindow* window, double xPos, double yPos)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    float x = static_cast<float>(xPos);
    float y = static_cast<float>(yPos);

    if (self->mouseFirstMove) {
        self->lastX = x;
        self->lastY = y;
        self->mouseFirstMove = false;
    }

    // Acumula: podem chegar vários eventos entre dois frames
    self->xChange += x - self->lastX;
    self->yChange += self->lastY - y; // eixo Y da tela é invertido

    self->lastX = x;
    self->lastY = y;
}

void Window::HandleFramebufferSize(GLFWwindow* window, int width, int height)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    self->bufferWidth = width;
    self->bufferHeight = height;
    glViewport(0, 0, width, height);
}
