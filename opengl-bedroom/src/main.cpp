#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include "Shader.h"
#include "Camera.h"

const float ROOM_W = 4.0f;
const float ROOM_H = 3.0f;
const float ROOM_D = 4.0f;

Camera camera(glm::vec3(0.0f, 1.6f, 1.5f));
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
    glm::vec2 uvScale;
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

void drawSurfaces(const Shader& shader, unsigned int vao, int vertexCount, const std::vector<Surface>& surfaces)
{
    glBindVertexArray(vao);
    for (const Surface& s : surfaces)
    {
        shader.setMat4("model", s.model);
        shader.setVec2("uvScale", s.uvScale);
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Bedroom", nullptr, nullptr);
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
    stbi_set_flip_vertically_on_load(true);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    unsigned int quad = createQuad();
    unsigned int cube = createCube();

    unsigned int floorTex = loadTexture("assets/textures/floor.jpg");
    unsigned int wallTex = loadTexture("assets/textures/wall.jpg");
    unsigned int whiteTex = makeColorTexture(235, 235, 230);
    unsigned int blueTex = makeColorTexture(90, 120, 170);
    unsigned int darkTex = makeColorTexture(60, 60, 65);
    unsigned int creamTex = makeColorTexture(240, 225, 190);
    unsigned int bulbTex = makeColorTexture(255, 240, 200);

    const float W = ROOM_W, H = ROOM_H, D = ROOM_D;
    std::vector<Surface> room = {
        { makeModel(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), floorTex, glm::vec2(W, D) },
        { makeModel(glm::vec3(0.0f, H, 0.0f), 180.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, D)), wallTex, glm::vec2(W, D) },
        { makeModel(glm::vec3(0.0f, H / 2, -D / 2), 90.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, H)), wallTex, glm::vec2(W, H) },
        { makeModel(glm::vec3(0.0f, H / 2, D / 2), -90.0f, glm::vec3(1, 0, 0), glm::vec3(W, 1, H)), wallTex, glm::vec2(W, H) },
        { makeModel(glm::vec3(-W / 2, H / 2, 0.0f), -90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) },
        { makeModel(glm::vec3(W / 2, H / 2, 0.0f), 90.0f, glm::vec3(0, 0, 1), glm::vec3(H, 1, D)), wallTex, glm::vec2(H, D) }
    };

    std::vector<Surface> furniture = {
        { boxModel(glm::vec3(-1.4f, 0.175f, -1.0f), glm::vec3(1.2f, 0.35f, 2.0f)), floorTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(-1.4f, 0.45f, -0.95f), glm::vec3(1.1f, 0.2f, 1.85f)), blueTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(-1.4f, 0.61f, -1.7f), glm::vec3(0.8f, 0.12f, 0.35f)), whiteTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(-1.4f, 0.5f, -1.96f), glm::vec3(1.2f, 1.0f, 0.08f)), floorTex, glm::vec2(1.0f, 1.0f) },

        { boxModel(glm::vec3(1.7f, 0.73f, -1.0f), glm::vec3(0.6f, 0.04f, 1.2f)), floorTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.45f, 0.355f, -1.55f), glm::vec3(0.05f, 0.71f, 0.05f)), floorTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.95f, 0.355f, -1.55f), glm::vec3(0.05f, 0.71f, 0.05f)), floorTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.45f, 0.355f, -0.45f), glm::vec3(0.05f, 0.71f, 0.05f)), floorTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.95f, 0.355f, -0.45f), glm::vec3(0.05f, 0.71f, 0.05f)), floorTex, glm::vec2(1.0f, 1.0f) },

        { boxModel(glm::vec3(1.1f, 0.425f, -1.0f), glm::vec3(0.45f, 0.05f, 0.45f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(0.9f, 0.2f, -1.2f), glm::vec3(0.04f, 0.4f, 0.04f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.3f, 0.2f, -1.2f), glm::vec3(0.04f, 0.4f, 0.04f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(0.9f, 0.2f, -0.8f), glm::vec3(0.04f, 0.4f, 0.04f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.3f, 0.2f, -0.8f), glm::vec3(0.04f, 0.4f, 0.04f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(0.895f, 0.675f, -1.0f), glm::vec3(0.04f, 0.45f, 0.45f)), darkTex, glm::vec2(1.0f, 1.0f) },

        { boxModel(glm::vec3(1.8f, 0.765f, -1.4f), glm::vec3(0.15f, 0.03f, 0.15f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.8f, 0.955f, -1.4f), glm::vec3(0.025f, 0.35f, 0.025f)), darkTex, glm::vec2(1.0f, 1.0f) },
        { boxModel(glm::vec3(1.8f, 1.19f, -1.4f), glm::vec3(0.22f, 0.12f, 0.22f)), creamTex, glm::vec2(1.0f, 1.0f) }
    };

    std::vector<Surface> bulb = {
        { boxModel(glm::vec3(1.8f, 1.11f, -1.4f), glm::vec3(0.08f, 0.06f, 0.08f)), bulbTex, glm::vec2(1.0f, 1.0f) }
    };

    glm::vec3 ceilingPos(0.0f, H - 0.2f, 0.0f);
    glm::vec3 ceilingColor(1.0f, 0.95f, 0.85f);
    glm::vec3 lampPos(1.8f, 1.05f, -1.4f);
    glm::vec3 lampColor(1.0f, 0.75f, 0.45f);
    glm::vec3 bulbGlow(1.0f, 0.85f, 0.5f);

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

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("projection", glm::perspective(glm::radians(camera.fov), (float)fbW / fbH, 0.1f, 100.0f));
        shader.setMat4("view", camera.view());
        shader.setVec3("viewPos", camera.position);
        shader.setVec3("lightPos[0]", ceilingPos);
        shader.setVec3("lightColor[0]", ceilingColor);
        shader.setFloat("lightIntensity[0]", ceilingOn ? 1.0f : 0.0f);
        shader.setVec3("lightPos[1]", lampPos);
        shader.setVec3("lightColor[1]", lampColor);
        shader.setFloat("lightIntensity[1]", lampOn ? 1.5f : 0.0f);
        shader.setFloat("ambient", ceilingOn ? 0.15f : 0.04f);
        shader.setInt("tex", 0);
        glActiveTexture(GL_TEXTURE0);

        shader.setVec3("emissive", glm::vec3(0.0f));
        drawSurfaces(shader, quad, 6, room);
        drawSurfaces(shader, cube, 36, furniture);

        shader.setVec3("emissive", lampOn ? bulbGlow : glm::vec3(0.0f));
        drawSurfaces(shader, cube, 36, bulb);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}