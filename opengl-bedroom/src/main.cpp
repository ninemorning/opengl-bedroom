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
#include "Model.h"

const float ROOM_W = 7.0f;
const float ROOM_H = 3.0f;
const float ROOM_D = 5.5f;
const float WALL_TILE = 0.5f;
const int SNOW_COUNT = 5000;
const float SNOW_HEIGHT = 8.0f;
const float FURNITURE_SCALE = 1.8f;

Camera camera(glm::vec3(-1.0f, 1.6f, 0.8f));
float lastX = 640.0f;
float lastY = 360.0f;
bool firstMouse = true;

bool lampOn = true;
bool ceilingOn = true;
bool lWasDown = false;
bool kWasDown = false;
bool fWasDown = false;
bool eWasDown = false;

bool inBed = false;
bool onSofa = false;
bool doorOpen = false;
float doorAngle = 0.0f;
bool snowmanHugged = false;

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

struct Prop
{
    Model* model;
    glm::vec3 pos;
    float yaw;
    float scale = FURNITURE_SCALE;
};

const float DOOR_X0 = -1.65f;
const float DOOR_X1 = -0.75f;
const float DOOR_H = 2.1f;
const float PLAYER_RADIUS = 0.2f;

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    bool check(glm::vec3 pos, float r = PLAYER_RADIUS) const
    {
        return pos.x + r >= min.x && pos.x - r <= max.x &&
            pos.z + r >= min.z && pos.z - r <= max.z;
    }
};

const float WALL_T = 0.2f;

std::vector<AABB> colliders = {
    { glm::vec3(-ROOM_W / 2 - WALL_T, 0, -ROOM_D / 2 - WALL_T), glm::vec3(-ROOM_W / 2, ROOM_H, ROOM_D / 2 + WALL_T) },
    { glm::vec3(ROOM_W / 2, 0, -ROOM_D / 2 - WALL_T), glm::vec3(ROOM_W / 2 + WALL_T, ROOM_H, ROOM_D / 2 + WALL_T) },
    { glm::vec3(-ROOM_W / 2, 0, -ROOM_D / 2 - WALL_T), glm::vec3(ROOM_W / 2, ROOM_H, -ROOM_D / 2) },
    { glm::vec3(-ROOM_W / 2, 0, ROOM_D / 2), glm::vec3(DOOR_X0, ROOM_H, ROOM_D / 2 + WALL_T) },
    { glm::vec3(DOOR_X1, 0, ROOM_D / 2), glm::vec3(ROOM_W / 2, ROOM_H, ROOM_D / 2 + WALL_T) },
    { glm::vec3(2.3f, 0, -1.9f), glm::vec3(3.5f, ROOM_H, -0.1f) },
    { glm::vec3(2.0f, 0, 1.3f), glm::vec3(2.95f, ROOM_H, 2.25f) },
    { glm::vec3(0.5f, 0, 4.2f), glm::vec3(1.1f, 1.7f, 4.8f) },
    { glm::vec3(-5.0f, 0, 3.0f), glm::vec3(-4.0f, 2.4f, 4.0f) },
    { glm::vec3(4.4f, 0, 1.4f), glm::vec3(5.6f, 3.0f, 2.6f) },
    { glm::vec3(-6.2f, 0, -4.7f), glm::vec3(-4.8f, 3.5f, -3.3f) },
    { glm::vec3(5.45f, 0, -5.55f), glm::vec3(6.55f, 2.6f, -4.45f) }
};

const AABB doorCollider = { glm::vec3(DOOR_X0, 0, ROOM_D / 2 - 0.05f), glm::vec3(DOOR_X1, DOOR_H, ROOM_D / 2 + WALL_T) };

bool blocked(glm::vec3 pos)
{
    if (!doorOpen && doorCollider.check(pos)) return true;
    for (const AABB& box : colliders)
        if (box.check(pos)) return true;
    return false;
}

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

    if (keyPressedOnce(window, GLFW_KEY_E, eWasDown))
    {
        glm::vec3 doorPos((DOOR_X0 + DOOR_X1) / 2, 1.0f, ROOM_D / 2);
        if (glm::distance(camera.position, doorPos) < 3.0f)
        {
            doorOpen = !doorOpen;
        }
    }

    if (keyPressedOnce(window, GLFW_KEY_F, fWasDown))
    {
        glm::vec3 bedPos(-2.6f, 1.2f, -1.71f);
        glm::vec3 sofaPos(0.5f, 1.0f, -2.0f);
        glm::vec3 snowmanPos(0.8f, 1.0f, 4.5f);
        glm::vec3 deskLampPos(1.1f, 1.5f, 1.35f);

        float distBed = glm::distance(camera.position, bedPos);
        float distSofa = glm::distance(camera.position, sofaPos);
        float distSnowman = glm::distance(camera.position, snowmanPos);
        float distLamp = glm::distance(camera.position, deskLampPos);

        if (distLamp < 2.0f)
        {
            lampOn = !lampOn;
        }
        else if (distBed < 2.5f)
        {
            inBed = !inBed;
            onSofa = false;
            if (inBed) camera.position = glm::vec3(-2.6f, 1.25f, -1.71f);
            else camera.position = glm::vec3(-1.5f, 1.6f, -1.0f);
        }
        else if (distSofa < 2.2f)
        {
            onSofa = !onSofa;
            inBed = false;
            if (onSofa) camera.position = glm::vec3(0.5f, 1.15f, -1.7f);
            else camera.position = glm::vec3(0.5f, 1.6f, 0.3f);
        }
        else if (distSnowman < 2.0f)
        {
            snowmanHugged = !snowmanHugged;
        }
        else
        {
            lampOn = !lampOn;
        }
    }

    if (inBed || onSofa) return;

    glm::vec3 oldPos = camera.position;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(Move::Forward, dt);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(Move::Backward, dt);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(Move::Left, dt);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(Move::Right, dt);

    glm::vec3 target = camera.position;
    camera.position = oldPos;
    if (blocked(oldPos))
    {
        camera.position = target;
    }
    else
    {
        camera.position.x = target.x;
        if (blocked(camera.position)) camera.position.x = oldPos.x;
        camera.position.z = target.z;
        if (blocked(camera.position)) camera.position.z = oldPos.z;
    }
    camera.position.x = glm::clamp(camera.position.x, -12.0f, 12.0f);
    camera.position.z = glm::clamp(camera.position.z, -10.0f, 6.0f);

    if (keyPressedOnce(window, GLFW_KEY_L, lWasDown)) lampOn = !lampOn;
    if (keyPressedOnce(window, GLFW_KEY_K, kWasDown)) ceilingOn = !ceilingOn;
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
    std::uniform_real_distribution<float> distX(-15.0f, 15.0f);
    std::uniform_real_distribution<float> distY(0.0f, SNOW_HEIGHT);
    std::uniform_real_distribution<float> distZ(-25.0f, -3.0f);
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

glm::mat4 placeModel(const Model& m, glm::vec3 pos, float yaw, float scale)
{
    glm::vec3 pivot((m.boundsMin.x + m.boundsMax.x) / 2, m.boundsMin.y, (m.boundsMin.z + m.boundsMax.z) / 2);
    glm::mat4 t(1.0f);
    t = glm::translate(t, pos);
    t = glm::rotate(t, glm::radians(yaw), glm::vec3(0, 1, 0));
    t = glm::scale(t, glm::vec3(scale));
    t = glm::translate(t, -pivot);
    return t;
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

Surface frontWallPiece(float x0, float x1, float y0, float y1, unsigned int tex)
{
    glm::vec3 center((x0 + x1) / 2, (y0 + y1) / 2, ROOM_D / 2);
    glm::vec3 size(x1 - x0, 1.0f, y1 - y0);
    Surface s{ makeModel(center, -90.0f, glm::vec3(1, 0, 0), size), tex };
    s.uvScale = glm::vec2(size.x, size.z) * WALL_TILE;
    s.uvOffset = glm::vec2(x0 + ROOM_W / 2, y0) * WALL_TILE;
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Snow Cabin - Advanced Edition", nullptr, nullptr);
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
    unsigned int creamTex = makeColorTexture(230, 220, 200);
    unsigned int darkTex = makeColorTexture(60, 60, 65);
    unsigned int sootTex = makeColorTexture(20, 15, 12);
    unsigned int blackTex = makeColorTexture(0, 0, 0);

    unsigned int giftRedTex = makeColorTexture(200, 35, 35);
    unsigned int giftGreenTex = makeColorTexture(25, 135, 55);
    unsigned int giftGoldTex = makeColorTexture(235, 185, 30);
    unsigned int giftBlueTex = makeColorTexture(30, 80, 210);
    unsigned int ribbonGold = makeColorTexture(255, 215, 0);
    unsigned int ribbonWhite = makeColorTexture(245, 245, 245);

    unsigned int curtainVelvet = makeColorTexture(45, 75, 115);
    unsigned int curtainTrim = makeColorTexture(220, 185, 75);

    unsigned int snowmanWhite = makeColorTexture(245, 245, 250);
    unsigned int coalBlack = makeColorTexture(20, 20, 20);
    unsigned int carrotOrange = makeColorTexture(230, 110, 20);
    unsigned int scarfRed = makeColorTexture(210, 40, 40);
    unsigned int mountainTex = makeColorTexture(55, 75, 100);
    unsigned int pineGreenTex = makeColorTexture(20, 50, 32);
    unsigned int woodDoorTex = makeColorTexture(110, 65, 35);
    unsigned int brassKnobTex = makeColorTexture(200, 175, 80);
    unsigned int paintingTex = makeColorTexture(180, 150, 110);

    Model tree;
    tree.load("assets/models/christmas_tree/obj.obj");
    tree.setEmissive("Mi_161", glm::vec3(1.2f, 0.9f, 0.4f));

    Model bed, nightstand, sofa, rug, coffeeTable, ceilingLamp, desk, chair, deskLamp, books, bookcase, plant;
    bed.load("assets/models/furniture/bedDouble.obj");
    nightstand.load("assets/models/furniture/cabinetBedDrawerTable.obj");
    sofa.load("assets/models/furniture/loungeSofa.obj");
    rug.load("assets/models/furniture/rugRectangle.obj");
    coffeeTable.load("assets/models/furniture/tableCoffee.obj");
    ceilingLamp.load("assets/models/furniture/lampSquareCeiling.obj");
    desk.load("assets/models/furniture/desk.obj");
    chair.load("assets/models/furniture/chairDesk.obj");
    deskLamp.load("assets/models/furniture/lampSquareTable.obj");
    books.load("assets/models/furniture/books.obj");
    bookcase.load("assets/models/furniture/bookcaseClosedWide.obj");
    plant.load("assets/models/furniture/pottedPlant.obj");

    const float W = ROOM_W, H = ROOM_H, D = ROOM_D;
    const float K = FURNITURE_SCALE;
    const float lampScale = 3.0f;
    const float rugTop = rug.size().y * K;
    const float deskTop = desk.size().y * K;
    const float standTop = nightstand.size().y * K;
    const glm::vec3 standPos(-1.44f, 0.0f, -2.52f);

    std::vector<Prop> props = {
        { &bed, glm::vec3(-2.6f, 0.0f, -1.71f), 180.0f },
        { &nightstand, standPos, 180.0f },
        { &sofa, glm::vec3(0.5f, 0.0f, -2.0f), 180.0f },
        { &rug, glm::vec3(0.5f, 0.0f, -0.6f), 0.0f },
        { &coffeeTable, glm::vec3(0.5f, rugTop, -0.6f), 0.0f },
        { &ceilingLamp, glm::vec3(0.5f, H - ceilingLamp.size().y * lampScale, -0.6f), 0.0f, lampScale },
        { &desk, glm::vec3(0.6f, 0.0f, 1.5f), 180.0f },
        { &chair, glm::vec3(0.6f, 0.0f, 2.1f), 0.0f },
        { &deskLamp, glm::vec3(1.1f, deskTop, 1.35f), 0.0f },
        { &books, glm::vec3(0.15f, deskTop, 1.45f), 20.0f },
        { &bookcase, glm::vec3(-3.25f, 0.0f, 2.0f), -90.0f },
        { &plant, glm::vec3(1.75f, 0.0f, -2.2f), 0.0f }
    };

    for (const Prop& p : props)
    {
        if (p.model == &rug || p.model == &ceilingLamp || p.model == &deskLamp || p.model == &books) continue;
        glm::mat4 m = placeModel(*p.model, p.pos, p.yaw, p.scale);
        AABB box = { glm::vec3(1e30f), glm::vec3(-1e30f) };
        for (int i = 0; i < 8; i++)
        {
            glm::vec3 c((i & 1) ? p.model->boundsMax.x : p.model->boundsMin.x,
                (i & 2) ? p.model->boundsMax.y : p.model->boundsMin.y,
                (i & 4) ? p.model->boundsMax.z : p.model->boundsMin.z);
            glm::vec3 w = glm::vec3(m * glm::vec4(c, 1.0f));
            box.min = glm::min(box.min, w);
            box.max = glm::max(box.max, w);
        }
        colliders.push_back(box);
    }
    const float winX0 = -1.3f, winX1 = 2.3f, winY0 = 0.9f, winY1 = 2.4f;

    Surface floorS{ makeModel(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), floorTex, glm::vec2(W, D) * 0.5f };
    Surface ceilS{ makeModel(glm::vec3(0.0f, H, 0.0f), 180.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), wallTex, glm::vec2(W, D) * WALL_TILE };
    Surface leftS{ makeModel(glm::vec3(-W / 2, H / 2, 0.0f), -90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) * WALL_TILE };
    Surface rightS{ makeModel(glm::vec3(W / 2, H / 2, 0.0f), 90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) * WALL_TILE };

    std::vector<Surface> room = {
        floorS, ceilS, leftS, rightS,
        frontWallPiece(-W / 2, DOOR_X0, 0.0f, H, wallTex),
        frontWallPiece(DOOR_X1, W / 2, 0.0f, H, wallTex),
        frontWallPiece(DOOR_X0, DOOR_X1, DOOR_H, H, wallTex),
        backWallPiece(-W / 2, winX0, 0.0f, H, wallTex),
        backWallPiece(winX1, W / 2, 0.0f, H, wallTex),
        backWallPiece(winX0, winX1, 0.0f, winY0, wallTex),
        backWallPiece(winX0, winX1, winY1, H, wallTex)
    };

    Surface ground{ makeModel(glm::vec3(0.0f, -0.02f, 0.0f), 0.0f, glm::vec3(1, 0, 0), glm::vec3(40.0f, 1.0f, 40.0f)), blackTex };
    ground.emissive = glm::vec3(0.38f, 0.45f, 0.60f);
    Surface sky{ makeModel(glm::vec3(0.0f, 8.0f, -15.0f), 90.0f, glm::vec3(1, 0, 0), glm::vec3(50.0f, 1.0f, 20.0f)), blackTex };
    sky.emissive = glm::vec3(0.02f, 0.04f, 0.12f);

    std::vector<Surface> outside = {
        ground, sky,
        box(glm::vec3(-10.0f, 3.5f, -18.0f), glm::vec3(16.0f, 7.0f, 2.0f), mountainTex),
        box(glm::vec3(10.0f, 4.0f, -20.0f), glm::vec3(18.0f, 8.0f, 2.0f), mountainTex),
        box(glm::vec3(-4.5f, 1.2f, 3.5f), glm::vec3(1.0f, 2.4f, 1.0f), pineGreenTex),
        box(glm::vec3(5.0f, 1.5f, 2.0f), glm::vec3(1.2f, 3.0f, 1.2f), pineGreenTex),
        box(glm::vec3(-5.5f, 1.8f, -4.0f), glm::vec3(1.4f, 3.5f, 1.4f), pineGreenTex),
        box(glm::vec3(6.0f, 1.3f, -5.0f), glm::vec3(1.1f, 2.6f, 1.1f), pineGreenTex),

        box(glm::vec3(0.8f, 0.35f, 4.5f), glm::vec3(0.6f, 0.6f, 0.6f), snowmanWhite),
        box(glm::vec3(0.8f, 0.85f, 4.5f), glm::vec3(0.45f, 0.45f, 0.45f), snowmanWhite),
        box(glm::vec3(0.8f, 1.25f, 4.5f), glm::vec3(0.32f, 0.32f, 0.32f), snowmanWhite),
        box(glm::vec3(0.72f, 1.30f, 4.67f), glm::vec3(0.05f, 0.05f, 0.05f), coalBlack),
        box(glm::vec3(0.88f, 1.30f, 4.67f), glm::vec3(0.05f, 0.05f, 0.05f), coalBlack),
        box(glm::vec3(0.8f, 1.25f, 4.72f), glm::vec3(0.06f, 0.06f, 0.15f), carrotOrange),
        box(glm::vec3(0.8f, 1.08f, 4.5f), glm::vec3(0.48f, 0.1f, 0.48f), scarfRed),
        box(glm::vec3(0.8f, 1.50f, 4.5f), glm::vec3(0.28f, 0.25f, 0.28f), coalBlack)
    };
    outside[2].emissive = glm::vec3(0.05f, 0.08f, 0.15f);
    outside[3].emissive = glm::vec3(0.06f, 0.09f, 0.18f);

    std::vector<Surface> furniture = {
        box(glm::vec3(standPos.x, standTop + 0.05f, standPos.z), glm::vec3(0.06f, 0.1f, 0.06f), creamTex),

        box(glm::vec3(winX0 / 2 + winX1 / 2, 0.875f, -2.6f), glm::vec3(3.8f, 0.05f, 0.3f), floorTex),
        box(glm::vec3(winX0, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(-0.4f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(0.5f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(1.4f, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(winX1, 1.65f, -2.75f), glm::vec3(0.08f, 1.5f, 0.15f), floorTex),
        box(glm::vec3(0.5f, winY1, -2.75f), glm::vec3(3.68f, 0.08f, 0.15f), floorTex),

        box(glm::vec3(0.5f, 2.47f, -2.72f), glm::vec3(4.1f, 0.04f, 0.06f), curtainTrim),
        box(glm::vec3(-1.18f, 1.55f, -2.68f), glm::vec3(0.42f, 1.7f, 0.07f), curtainVelvet),
        box(glm::vec3(-1.38f, 1.55f, -2.68f), glm::vec3(0.12f, 1.7f, 0.08f), curtainTrim),
        box(glm::vec3(2.18f, 1.55f, -2.68f), glm::vec3(0.42f, 1.7f, 0.07f), curtainVelvet),
        box(glm::vec3(2.38f, 1.55f, -2.68f), glm::vec3(0.12f, 1.7f, 0.08f), curtainTrim),
        box(glm::vec3(0.5f, 2.34f, -2.68f), glm::vec3(3.95f, 0.22f, 0.08f), curtainVelvet),
        box(glm::vec3(0.5f, 2.22f, -2.68f), glm::vec3(3.95f, 0.05f, 0.09f), curtainTrim),

        box(glm::vec3(3.15f, 1.5f, -1.0f), glm::vec3(0.7f, 3.0f, 1.8f), stoneTex),
        box(glm::vec3(2.55f, 0.06f, -1.0f), glm::vec3(0.5f, 0.12f, 2.0f), stoneTex),
        box(glm::vec3(2.79f, 0.42f, -1.0f), glm::vec3(0.02f, 0.6f, 0.9f), sootTex),
        box(glm::vec3(2.62f, 1.2f, -1.0f), glm::vec3(0.35f, 0.08f, 2.0f), floorTex),
        box(glm::vec3(2.6f, 0.27f, -1.15f), glm::vec3(0.12f, 0.1f, 0.6f), floorTex),
        box(glm::vec3(2.6f, 0.27f, -0.85f), glm::vec3(0.12f, 0.1f, 0.6f), floorTex),

        box(glm::vec3(-3.47f, 1.8f, 0.0f), glm::vec3(0.04f, 0.9f, 1.2f), paintingTex)
    };

    std::vector<Surface> giftBoxes = {
        box(glm::vec3(2.15f, 0.14f, 1.55f), glm::vec3(0.28f, 0.28f, 0.28f), giftRedTex),
        box(glm::vec3(2.15f, 0.142f, 1.55f), glm::vec3(0.29f, 0.29f, 0.05f), ribbonGold),
        box(glm::vec3(2.15f, 0.142f, 1.55f), glm::vec3(0.05f, 0.29f, 0.29f), ribbonGold),

        box(glm::vec3(2.75f, 0.12f, 1.85f), glm::vec3(0.35f, 0.24f, 0.28f), giftGreenTex),
        box(glm::vec3(2.75f, 0.122f, 1.85f), glm::vec3(0.36f, 0.25f, 0.05f), ribbonWhite),
        box(glm::vec3(2.75f, 0.122f, 1.85f), glm::vec3(0.05f, 0.25f, 0.29f), ribbonWhite),

        box(glm::vec3(2.45f, 0.15f, 1.45f), glm::vec3(0.30f, 0.30f, 0.30f), giftGoldTex),
        box(glm::vec3(2.45f, 0.152f, 1.45f), glm::vec3(0.31f, 0.31f, 0.05f), giftRedTex),
        box(glm::vec3(2.45f, 0.152f, 1.45f), glm::vec3(0.05f, 0.31f, 0.31f), giftRedTex),

        box(glm::vec3(2.20f, 0.11f, 2.10f), glm::vec3(0.26f, 0.22f, 0.26f), giftBlueTex),
        box(glm::vec3(2.20f, 0.112f, 2.10f), glm::vec3(0.27f, 0.23f, 0.04f), ribbonWhite),

        box(glm::vec3(2.15f, 0.34f, 1.55f), glm::vec3(0.18f, 0.18f, 0.18f), giftGoldTex),
        box(glm::vec3(2.15f, 0.342f, 1.55f), glm::vec3(0.19f, 0.19f, 0.03f), giftRedTex)
    };

    std::vector<Surface> glows = {
        box(glm::vec3(2.65f, 0.17f, -1.0f), glm::vec3(0.3f, 0.1f, 0.6f), blackTex),
        box(glm::vec3(standPos.x, standTop + 0.125f, standPos.z), glm::vec3(0.03f, 0.05f, 0.03f), blackTex)
    };
    glows[1].emissive = glm::vec3(2.0f, 1.3f, 0.5f);

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

    glm::vec3 ceilingPos(0.5f, 2.3f, -0.6f);
    glm::vec3 ceilingColor(1.0f, 0.88f, 0.7f);
    glm::vec3 lampPos(1.1f, deskTop + 0.4f, 1.35f);
    glm::vec3 lampColor(1.0f, 0.75f, 0.45f);
    glm::vec3 firePos(2.45f, 0.55f, -1.0f);
    glm::vec3 fireColor(1.0f, 0.5f, 0.18f);
    glm::vec3 shadeGlow(1.0f, 0.85f, 0.55f);

    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;
        processInput(window, dt);

        float targetAngle = doorOpen ? 90.0f : 0.0f;
        doorAngle += (targetAngle - doorAngle) * (dt * 6.0f);

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH == 0)
        {
            glfwPollEvents();
            continue;
        }

        float flicker = 0.35f * std::sin(now * 7.3f) + 0.2f * std::sin(now * 13.1f + 1.3f) + 0.1f * std::sin(now * 23.7f);
        glows[0].emissive = fireColor * (2.0f + 0.5f * flicker);
        deskLamp.setEmissive("lamp", lampOn ? shadeGlow : glm::vec3(0.0f));
        ceilingLamp.setEmissive("lamp", ceilingOn ? shadeGlow : glm::vec3(0.0f));

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

        for (int i = 0; i < 4; i++)
        {
            float phase = now * 3.0f + i * 1.57f;
            glm::vec3 treeLightColor = glm::vec3(
                0.5f + 0.5f * std::sin(phase),
                0.5f + 0.5f * std::sin(phase + 2.09f),
                0.5f + 0.5f * std::sin(phase + 4.18f)
            );
            std::string posName = "lightPos[" + std::to_string(3 + i) + "]";
            std::string colName = "lightColor[" + std::to_string(3 + i) + "]";
            std::string intName = "lightIntensity[" + std::to_string(3 + i) + "]";

            shader.setVec3(posName.c_str(), glm::vec3(2.5f + (i % 2) * 0.4f - 0.2f, 1.0f + i * 0.3f, 1.9f + (i / 2) * 0.4f - 0.2f));
            shader.setVec3(colName.c_str(), treeLightColor);
            shader.setFloat(intName.c_str(), 0.35f);
        }

        shader.setFloat("ambient", ceilingOn ? 0.15f : 0.04f);
        shader.setFloat("exposure", 1.1f);
        shader.setFloat("alpha", 1.0f);
        shader.setInt("tex", 0);
        glActiveTexture(GL_TEXTURE0);

        drawSurfaces(shader, quad, 6, outside);
        drawSurfaces(shader, quad, 6, room);
        drawSurfaces(shader, cube, 36, furniture);

        {
            float doorW = DOOR_X1 - DOOR_X0;
            glm::mat4 hinge = glm::mat4(1.0f);
            hinge = glm::translate(hinge, glm::vec3(DOOR_X0, DOOR_H / 2, D / 2 - 0.03f));
            hinge = glm::rotate(hinge, glm::radians(doorAngle), glm::vec3(0, 1, 0));
            glm::mat4 doorMat = glm::translate(hinge, glm::vec3(doorW / 2, 0.0f, 0.0f));
            doorMat = glm::scale(doorMat, glm::vec3(doorW, DOOR_H, 0.06f));
            shader.setMat4("model", doorMat);
            shader.setVec2("uvScale", glm::vec2(1.0f));
            shader.setVec2("uvOffset", glm::vec2(0.0f));
            shader.setVec3("emissive", glm::vec3(0.0f));
            glBindTexture(GL_TEXTURE_2D, woodDoorTex);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 knobMat = glm::translate(hinge, glm::vec3(doorW - 0.1f, -0.05f, -0.05f));
            knobMat = glm::scale(knobMat, glm::vec3(0.06f, 0.06f, 0.06f));
            shader.setMat4("model", knobMat);
            glBindTexture(GL_TEXTURE_2D, brassKnobTex);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (const Prop& p : props)
            p.model->draw(shader, placeModel(*p.model, p.pos, p.yaw, p.scale));

        tree.draw(shader, makeModel(glm::vec3(2.5f, 0.0f, 1.9f), 0.0f, glm::vec3(0, 1, 0), glm::vec3(0.22f)));
        drawSurfaces(shader, cube, 36, giftBoxes);
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