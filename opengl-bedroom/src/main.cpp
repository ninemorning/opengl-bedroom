#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>
#include <cmath>
#include <iostream>
#include "Shader.h"
#include "Camera.h"

const float ROOM_W = 7.0f;
const float ROOM_H = 3.0f;
const float ROOM_D = 5.5f;
const float WALL_TILE = 0.5f;
const int SNOW_COUNT = 4000;
const float SNOW_HEIGHT = 8.0f;

Camera camera(glm::vec3(-1.0f, 1.6f, 0.8f));
float lastX = 640.0f;
float lastY = 360.0f;
bool firstMouse = true;

bool lampOn = true;
bool ceilingOn = true;
bool lWasDown = false;
bool kWasDown = false;

struct Surface
{
    glm::mat4 model;
    unsigned int texture;
    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec3 emissive = glm::vec3(0.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f);
};

struct Flame
{
    glm::vec3 center;
    glm::vec2 size;
    float seed;
};

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    float x = (float)xpos;
    float y = (float)ypos;
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }
    camera.look(x - lastX, lastY - y);
    lastX = x;
    lastY = y;
}

bool keyPressedOnce(GLFWwindow* window, int key, bool& wasDown)
{
    bool down = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = down && !wasDown;
    wasDown = down;
    return pressed;
}

void processInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(Move::Forward, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(Move::Backward, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(Move::Left, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(Move::Right, dt);

    if (keyPressedOnce(window, GLFW_KEY_L, lWasDown)) lampOn = !lampOn;
    if (keyPressedOnce(window, GLFW_KEY_K, kWasDown)) ceilingOn = !ceilingOn;

    float margin = 0.3f;
    camera.position.x = glm::clamp(camera.position.x, -ROOM_W / 2 + margin, ROOM_W / 2 - margin);
    camera.position.z = glm::clamp(camera.position.z, -ROOM_D / 2 + margin, ROOM_D / 2 - margin);
}

unsigned int loadTexture(const char* path)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 3);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture: " << path << std::endl;
        unsigned char gray[3] = { 180, 180, 180 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, gray);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

unsigned int makeColorTexture(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char c[3] = { r, g, b };
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, c);
    return tex;
}

unsigned int createVAO(const float* vertices, size_t size)
{
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return vao;
}

unsigned int createQuad()
{
    float vertices[] = {
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };
    return createVAO(vertices, sizeof(vertices));
}

unsigned int createCube()
{
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };
    return createVAO(vertices, sizeof(vertices));
}

unsigned int createBillboard()
{
    float vertices[] = {
        -0.5f, 0.0f,
         0.5f, 0.0f,
         0.5f, 1.0f,
         0.5f, 1.0f,
        -0.5f, 1.0f,
        -0.5f, 0.0f
    };
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return vao;
}

unsigned int createSnow(int count)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distX(-10.0f, 10.0f);
    std::uniform_real_distribution<float> distY(0.0f, SNOW_HEIGHT);
    std::uniform_real_distribution<float> distZ(-15.0f, -3.0f);
    std::uniform_real_distribution<float> distSpeed(0.4f, 1.0f);
    std::uniform_real_distribution<float> distPhase(0.0f, 6.2832f);

    std::vector<float> data;
    data.reserve(count * 5);
    for (int i = 0; i < count; i++)
    {
        data.push_back(distX(rng));
        data.push_back(distY(rng));
        data.push_back(distZ(rng));
        data.push_back(distSpeed(rng));
        data.push_back(distPhase(rng));
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

glm::mat4 makeModel(glm::vec3 t, float angle, glm::vec3 axis, glm::vec3 s)
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, t);
    m = glm::rotate(m, glm::radians(angle), axis);
    m = glm::scale(m, s);
    return m;
}

glm::mat4 boxModel(glm::vec3 center, glm::vec3 size)
{
    glm::mat4 m(1.0f);
    m = glm::translate(m, center);
    m = glm::scale(m, size);
    return m;
}

Surface box(glm::vec3 center, glm::vec3 size, unsigned int tex)
{
    return Surface{ boxModel(center, size), tex };
}

Surface backWallPiece(float x0, float x1, float y0, float y1, unsigned int tex)
{
    glm::vec3 center((x0 + x1) / 2, (y0 + y1) / 2, -ROOM_D / 2);
    glm::vec3 size(x1 - x0, 1.0f, y1 - y0);
    Surface s{ makeModel(center, 90.0f, glm::vec3(1, 0, 0), size), tex };
    s.uvScale = glm::vec2(size.x, size.z) * WALL_TILE;
    s.uvOffset = glm::vec2(x0 + ROOM_W / 2, ROOM_H - y1) * WALL_TILE;
    return s;
}

void drawSurfaces(const Shader& shader, unsigned int vao, int vertexCount, const std::vector<Surface>& surfaces)
{
    glBindVertexArray(vao);
    for (const Surface& s : surfaces)
    {
        shader.setMat4("model", s.model);
        shader.setVec2("uvScale", s.uvScale);
        shader.setVec2("uvOffset", s.uvOffset);
        shader.setVec3("emissive", s.emissive);
        glBindTexture(GL_TEXTURE_2D, s.texture);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Snow Cabin", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    stbi_set_flip_vertically_on_load(true);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Shader flameShader("shaders/flame.vert", "shaders/flame.frag");
    Shader snowShader("shaders/snow.vert", "shaders/snow.frag");
    unsigned int quad = createQuad();
    unsigned int cube = createCube();
    unsigned int billboard = createBillboard();
    unsigned int snow = createSnow(SNOW_COUNT);

    unsigned int floorTex = loadTexture("assets/textures/floor.jpg");
    unsigned int wallTex = loadTexture("assets/textures/wall.jpg");
    unsigned int stoneTex = loadTexture("assets/textures/stone.jpg");
    unsigned int whiteTex = makeColorTexture(235, 235, 230);
    unsigned int creamTex = makeColorTexture(230, 220, 200);
    unsigned int redTex = makeColorTexture(150, 35, 35);
    unsigned int darkTex = makeColorTexture(60, 60, 65);
    unsigned int sootTex = makeColorTexture(20, 15, 12);
    unsigned int rugTex = makeColorTexture(210, 190, 165);
    unsigned int blackTex = makeColorTexture(0, 0, 0);
    unsigned int bulbTex = makeColorTexture(255, 240, 200);

    const float W = ROOM_W, H = ROOM_H, D = ROOM_D;
    const float winX0 = -1.3f, winX1 = 2.3f, winY0 = 0.9f, winY1 = 2.4f;

    Surface floorS{ makeModel(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), floorTex, glm::vec2(W, D) * 0.5f };
    Surface ceilS{ makeModel(glm::vec3(0.0f, H, 0.0f), 180.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), wallTex, glm::vec2(W, D) * WALL_TILE };
    Surface frontS{ makeModel(glm::vec3(0.0f, H / 2, D / 2), -90.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, H)), wallTex, glm::vec2(W, H) * WALL_TILE };
    Surface leftS{ makeModel(glm::vec3(-W / 2, H / 2, 0.0f), -90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) * WALL_TILE };
    Surface rightS{ makeModel(glm::vec3(W / 2, H / 2, 0.0f), 90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) * WALL_TILE };

    std::vector<Surface> room = {
        floorS, ceilS, frontS, leftS, rightS,
        backWallPiece(-W / 2, winX0, 0.0f, H, wallTex),
        backWallPiece(winX1, W / 2, 0.0f, H, wallTex),
        backWallPiece(winX0, winX1, 0.0f, winY0, wallTex),
        backWallPiece(winX0, winX1, winY1, H, wallTex)
    };

    Surface ground{ makeModel(glm::vec3(0.0f, -0.01f, -11.375f), 0.0f, glm::vec3(1, 0, 0), glm::vec3(40.0f, 1.0f, 17.25f)), blackTex };
    ground.emissive = glm::vec3(0.40f, 0.45f, 0.58f);
    Surface sky{ makeModel(glm::vec3(0.0f, 8.0f, -20.0f), 90.0f, glm::vec3(1, 0, 0), glm::vec3(40.0f, 1.0f, 20.0f)), blackTex };
    sky.emissive = glm::vec3(0.03f, 0.06f, 0.16f);
    std::vector<Surface> outside = { ground, sky };

    std::vector<Surface> furniture = {
        box(glm::vec3(-2.65f, 0.175f, -1.7f), glm::vec3(1.6f, 0.35f, 2.1f), floorTex),
        box(glm::vec3(-2.65f, 0.45f, -1.62f), glm::vec3(1.5f, 0.2f, 1.95f), whiteTex),
        box(glm::vec3(-2.65f, 0.57f, -1.245f), glm::vec3(1.54f, 0.04f, 1.2f), redTex),
        box(glm::vec3(-3.0f, 0.62f, -2.4f), glm::vec3(0.6f, 0.14f, 0.35f), whiteTex),
        box(glm::vec3(-2.3f, 0.62f, -2.4f), glm::vec3(0.6f, 0.14f, 0.35f), whiteTex),
        box(glm::vec3(-2.65f, 0.55f, -2.71f), glm::vec3(1.6f, 1.1f, 0.08f), floorTex),

        box(glm::vec3(-1.55f, 0.25f, -2.5f), glm::vec3(0.45f, 0.5f, 0.4f), floorTex),
        box(glm::vec3(-1.55f, 0.55f, -2.5f), glm::vec3(0.06f, 0.1f, 0.06f), creamTex),

        box(glm::vec3(0.5f, 0.21f, -1.85f), glm::vec3(2.0f, 0.42f, 0.85f), creamTex),
        box(glm::vec3(0.5f, 0.645f, -2.175f), glm::vec3(2.0f, 0.45f, 0.2f), creamTex),
        box(glm::vec3(-0.4f, 0.3f, -1.85f), glm::vec3(0.2f, 0.6f, 0.85f), creamTex),
        box(glm::vec3(1.4f, 0.3f, -1.85f), glm::vec3(0.2f, 0.6f, 0.85f), creamTex),
        box(glm::vec3(0.0f, 0.58f, -2.0f), glm::vec3(0.4f, 0.35f, 0.12f), redTex),
        box(glm::vec3(1.0f, 0.58f, -2.0f), glm::vec3(0.4f, 0.35f, 0.12f), whiteTex),

        box(glm::vec3(0.5f, 0.005f, -0.6f), glm::vec3(2.6f, 0.01f, 2.0f), rugTex),
        box(glm::vec3(0.5f, 0.45f, -0.6f), glm::vec3(0.8f, 0.05f, 0.8f), floorTex),
        box(glm::vec3(0.5f, 0.2f, -0.6f), glm::vec3(0.12f, 0.4f, 0.12f), floorTex),

        box(glm::vec3(0.5f, 2.775f, -0.6f), glm::vec3(0.03f, 0.45f, 0.03f), darkTex),
        box(glm::vec3(0.5f, 2.46f, -0.6f), glm::vec3(0.55f, 0.18f, 0.55f), creamTex),

        box(glm::vec3(winX0 / 2 + winX1 / 2, 0.875f, -2.6f), glm::vec3(3.8f, 0.05f, 0.3f), floorTex),
        box(glm::vec3(winX0, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(-0.4f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(0.5f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(1.4f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(winX1, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(0.5f, winY1, -2.75f), glm::vec3(3.68f, 0.08f, 0.15f), floorTex),

        box(glm::vec3(3.15f, 1.5f, -1.0f), glm::vec3(0.7f, 3.0f, 1.8f), stoneTex),
        box(glm::vec3(2.55f, 0.06f, -1.0f), glm::vec3(0.5f, 0.12f, 2.0f), stoneTex),
        box(glm::vec3(2.79f, 0.42f, -1.0f), glm::vec3(0.02f, 0.6f, 0.9f), sootTex),
        box(glm::vec3(2.62f, 1.2f, -1.0f), glm::vec3(0.35f, 0.08f, 2.0f), floorTex),
        box(glm::vec3(2.6f, 0.27f, -1.15f), glm::vec3(0.12f, 0.1f, 0.6f), floorTex),
        box(glm::vec3(2.6f, 0.27f, -0.85f), glm::vec3(0.12f, 0.1f, 0.6f), floorTex),

        box(glm::vec3(0.6f, 0.73f, 1.5f), glm::vec3(1.4f, 0.04f, 0.65f), floorTex),
        box(glm::vec3(-0.05f, 0.355f, 1.22f), glm::vec3(0.05f, 0.71f, 0.05f), floorTex),
        box(glm::vec3(1.25f, 0.355f, 1.22f), glm::vec3(0.05f, 0.71f, 0.05f), floorTex),
        box(glm::vec3(-0.05f, 0.355f, 1.78f), glm::vec3(0.05f, 0.71f, 0.05f), floorTex),
        box(glm::vec3(1.25f, 0.355f, 1.78f), glm::vec3(0.05f, 0.71f, 0.05f), floorTex),

        box(glm::vec3(0.6f, 0.425f, 2.1f), glm::vec3(0.45f, 0.05f, 0.45f), floorTex),
        box(glm::vec3(0.4f, 0.2f, 1.9f), glm::vec3(0.04f, 0.4f, 0.04f), floorTex),
        box(glm::vec3(0.8f, 0.2f, 1.9f), glm::vec3(0.04f, 0.4f, 0.04f), floorTex),
        box(glm::vec3(0.4f, 0.2f, 2.3f), glm::vec3(0.04f, 0.4f, 0.04f), floorTex),
        box(glm::vec3(0.8f, 0.2f, 2.3f), glm::vec3(0.04f, 0.4f, 0.04f), floorTex),
        box(glm::vec3(0.6f, 0.675f, 2.305f), glm::vec3(0.45f, 0.45f, 0.04f), floorTex),

        box(glm::vec3(1.1f, 0.765f, 1.3f), glm::vec3(0.15f, 0.03f, 0.15f), darkTex),
        box(glm::vec3(1.1f, 0.955f, 1.3f), glm::vec3(0.025f, 0.35f, 0.025f), darkTex),
        box(glm::vec3(1.1f, 1.19f, 1.3f), glm::vec3(0.22f, 0.12f, 0.22f), creamTex),

        box(glm::vec3(-3.3f, 0.45f, 2.0f), glm::vec3(0.35f, 0.9f, 1.2f), floorTex),

        box(glm::vec3(-1.2f, 1.05f, 2.72f), glm::vec3(0.9f, 2.1f, 0.06f), floorTex),
        box(glm::vec3(-0.85f, 1.0f, 2.68f), glm::vec3(0.04f, 0.04f, 0.06f), darkTex)
    };

    std::vector<Surface> glows = {
        box(glm::vec3(1.1f, 1.11f, 1.3f), glm::vec3(0.08f, 0.06f, 0.08f), bulbTex),
        box(glm::vec3(2.65f, 0.17f, -1.0f), glm::vec3(0.3f, 0.1f, 0.6f), blackTex),
        box(glm::vec3(-1.55f, 0.625f, -2.5f), glm::vec3(0.03f, 0.05f, 0.03f), blackTex),
        box(glm::vec3(0.5f, 2.345f, -0.6f), glm::vec3(0.2f, 0.05f, 0.2f), bulbTex)
    };
    glows[2].emissive = glm::vec3(2.0f, 1.3f, 0.5f);

    std::vector<Flame> flames = {
        { glm::vec3(2.62f, 0.3f, -1.2f), glm::vec2(0.28f, 0.45f), 0.0f },
        { glm::vec3(2.60f, 0.3f, -1.0f), glm::vec2(0.35f, 0.6f), 3.7f },
        { glm::vec3(2.62f, 0.3f, -0.82f), glm::vec2(0.28f, 0.5f), 7.1f },
        { glm::vec3(2.64f, 0.3f, -1.1f), glm::vec2(0.2f, 0.35f), 11.3f },
        { glm::vec3(2.64f, 0.3f, -0.9f), glm::vec2(0.2f, 0.38f), 15.9f }
    };

    Surface glass{ makeModel(glm::vec3(0.5f, 1.65f, -2.75f), 90.0f, glm::vec3(1, 0, 0), glm::vec3(3.6f, 1.0f, 1.5f)), blackTex };
    glass.emissive = glm::vec3(0.02f, 0.03f, 0.06f);
    std::vector<Surface> glassList = { glass };

    glm::vec3 ceilingPos(0.5f, 2.25f, -0.6f);
    glm::vec3 ceilingColor(1.0f, 0.88f, 0.7f);
    glm::vec3 lampPos(1.1f, 1.05f, 1.3f);
    glm::vec3 lampColor(1.0f, 0.75f, 0.45f);
    glm::vec3 firePos(2.45f, 0.55f, -1.0f);
    glm::vec3 fireColor(1.0f, 0.5f, 0.18f);
    glm::vec3 bulbGlow(2.5f, 2.1f, 1.4f);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;
        processInput(window, dt);

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH == 0)
        {
            glfwPollEvents();
            continue;
        }

        float flicker = 0.35f * std::sin(now * 7.3f) + 0.2f * std::sin(now * 13.1f + 1.3f) + 0.1f * std::sin(now * 23.7f);
        glows[0].emissive = lampOn ? bulbGlow : glm::vec3(0.0f);
        glows[1].emissive = fireColor * (2.0f + 0.5f * flicker);
        glows[3].emissive = ceilingOn ? bulbGlow : glm::vec3(0.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), (float)fbW / fbH, 0.1f, 100.0f);
        glm::mat4 view = camera.view();

        glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.position);
        shader.setVec3("lightPos[0]", ceilingPos);
        shader.setVec3("lightColor[0]", ceilingColor);
        shader.setFloat("lightIntensity[0]", ceilingOn ? 1.0f : 0.0f);
        shader.setVec3("lightPos[1]", lampPos);
        shader.setVec3("lightColor[1]", lampColor);
        shader.setFloat("lightIntensity[1]", lampOn ? 1.2f : 0.0f);
        shader.setVec3("lightPos[2]", firePos);
        shader.setVec3("lightColor[2]", fireColor);
        shader.setFloat("lightIntensity[2]", 1.4f + flicker);
        shader.setFloat("ambient", ceilingOn ? 0.15f : 0.04f);
        shader.setFloat("exposure", 1.1f);
        shader.setFloat("alpha", 1.0f);
        shader.setInt("tex", 0);
        glActiveTexture(GL_TEXTURE0);

        drawSurfaces(shader, quad, 6, outside);
        drawSurfaces(shader, quad, 6, room);
        drawSurfaces(shader, cube, 36, furniture);
        drawSurfaces(shader, cube, 36, glows);

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        snowShader.use();
        snowShader.setMat4("projection", projection);
        snowShader.setMat4("view", view);
        snowShader.setFloat("time", now);
        snowShader.setFloat("baseY", 0.0f);
        snowShader.setFloat("height", SNOW_HEIGHT);
        snowShader.setFloat("pointScale", projection[1][1] * fbH * 0.5f);
        glBindVertexArray(snow);
        glDrawArrays(GL_POINTS, 0, SNOW_COUNT);

        glBlendFunc(GL_ONE, GL_ONE);
        flameShader.use();
        flameShader.setMat4("projection", projection);
        flameShader.setMat4("view", view);
        flameShader.setFloat("time", now);
        flameShader.setFloat("intensity", 1.2f + 0.3f * flicker);
        glBindVertexArray(billboard);
        for (const Flame& f : flames)
        {
            flameShader.setVec3("center", f.center);
            flameShader.setVec2("size", f.size);
            flameShader.setFloat("seed", f.seed);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        shader.use();
        shader.setFloat("alpha", 0.35f);
        drawSurfaces(shader, quad, 6, glassList);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
