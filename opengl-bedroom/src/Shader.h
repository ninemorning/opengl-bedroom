#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int id;

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vCode = readFile(vertexPath);
        std::string fCode = readFile(fragmentPath);
        unsigned int vs = compile(GL_VERTEX_SHADER, vCode.c_str(), vertexPath);
        unsigned int fs = compile(GL_FRAGMENT_SHADER, fCode.c_str(), fragmentPath);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        int ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetProgramInfoLog(id, 1024, nullptr, log);
            std::cout << "Shader link error:\n" << log << std::endl;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void use() const { glUseProgram(id); }
    void setInt(const char* name, int v) const { glUniform1i(glGetUniformLocation(id, name), v); }
    void setFloat(const char* name, float v) const { glUniform1f(glGetUniformLocation(id, name), v); }
    void setVec2(const char* name, const glm::vec2& v) const { glUniform2fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v)); }
    void setVec3(const char* name, const glm::vec3& v) const { glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v)); }
    void setMat4(const char* name, const glm::mat4& m) const { glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    static std::string readFile(const char* path)
    {
        std::ifstream file(path);
        if (!file)
            std::cout << "Cannot open shader: " << path << std::endl;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    static unsigned int compile(GLenum type, const char* src, const char* path)
    {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetShaderInfoLog(s, 1024, nullptr, log);
            std::cout << "Shader compile error (" << path << "):\n" << log << std::endl;
        }
        return s;
    }
};
