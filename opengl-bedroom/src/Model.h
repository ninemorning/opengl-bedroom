#pragma once
// Model.h - simple OBJ + MTL loader (one mesh per material)
// Vertex layout matches createVAO in main.cpp: position3 + normal3 + uv2
#include <glad/glad.h>
#include <glm/glm.hpp>
// NOTE: <stb_image.h> must be included BEFORE this file (main.cpp already does this)
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "Shader.h"

struct ObjData
{
    std::string mtlFile;                          // file name given by mtllib in the obj
    std::vector<std::string> materialNames;       // material names (same order as vertices)
    std::vector<std::vector<float>> vertices;     // one array per material: pos3 + normal3 + uv2
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);
};

struct MtlInfo
{
    glm::vec3 kd = glm::vec3(0.8f);
    std::string mapKd;                            // file name only (author's original path is stripped)
};

namespace modelDetail
{
    inline bool readFile(const std::string& path, std::string& out)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.seekg(0, std::ios::end);
        std::streamoff size = f.tellg();
        f.seekg(0, std::ios::beg);
        out.resize((size_t)size);
        f.read(&out[0], size);
        out.resize((size_t)f.gcount());
        return true;
    }

    inline const char* skipSpaces(const char* p)
    {
        while (*p == ' ' || *p == '\t') p++;
        return p;
    }

    inline std::string readWord(const char*& p)
    {
        p = skipSpaces(p);
        const char* s = p;
        while (*p && *p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') p++;
        return std::string(s, p);
    }

    inline std::string readRest(const char* p)
    {
        p = skipSpaces(p);
        const char* s = p;
        while (*p && *p != '\n' && *p != '\r') p++;
        while (p > s && (p[-1] == ' ' || p[-1] == '\t')) p--;
        return std::string(s, p);
    }

    inline std::string baseName(const std::string& path)
    {
        size_t pos = path.find_last_of("/\\");
        return pos == std::string::npos ? path : path.substr(pos + 1);
    }

    // read one face vertex "v/vt/vn" (also v//vn, v/vt, v)
    inline void readFaceVertex(const char*& p, int& v, int& vt, int& vn)
    {
        v = vt = vn = 0;
        p = skipSpaces(p);
        v = (int)std::strtol(p, (char**)&p, 10);
        if (*p == '/')
        {
            p++;
            if (*p != '/') vt = (int)std::strtol(p, (char**)&p, 10);
            if (*p == '/')
            {
                p++;
                vn = (int)std::strtol(p, (char**)&p, 10);
            }
        }
    }

    inline int fixIndex(int idx, size_t count)
    {
        if (idx > 0) return idx - 1;
        if (idx < 0) return (int)count + idx;
        return -1;
    }
}

// parse an obj file (no OpenGL needed)
inline bool parseObj(const std::string& path, ObjData& out)
{
    using namespace modelDetail;
    std::string text;
    if (!readFile(path, text))
    {
        std::cout << "Cannot open model: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> pos, nor;
    std::vector<glm::vec2> uv;
    pos.reserve(200000); nor.reserve(200000); uv.reserve(200000);

    std::map<std::string, size_t> matIndex;
    auto getMat = [&](const std::string& name) -> size_t
    {
        auto it = matIndex.find(name);
        if (it != matIndex.end()) return it->second;
        size_t i = out.materialNames.size();
        out.materialNames.push_back(name);
        out.vertices.emplace_back();
        matIndex[name] = i;
        return i;
    };
    size_t cur = getMat("default");

    const char* p = text.c_str();
    while (*p)
    {
        const char* line = skipSpaces(p);
        if (line[0] == 'v' && line[1] == ' ')
        {
            char* e;
            float x = std::strtof(line + 2, &e);
            float y = std::strtof(e, &e);
            float z = std::strtof(e, &e);
            pos.emplace_back(x, y, z);
            out.boundsMin = glm::min(out.boundsMin, pos.back());
            out.boundsMax = glm::max(out.boundsMax, pos.back());
        }
        else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ')
        {
            char* e;
            float u = std::strtof(line + 3, &e);
            float v = std::strtof(e, &e);
            uv.emplace_back(u, v);
        }
        else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ')
        {
            char* e;
            float x = std::strtof(line + 3, &e);
            float y = std::strtof(e, &e);
            float z = std::strtof(e, &e);
            nor.emplace_back(x, y, z);
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            const char* q = line + 2;
            int vi[64], ti[64], ni[64];
            int n = 0;
            while (n < 64)
            {
                q = skipSpaces(q);
                if (*q == '\n' || *q == '\r' || *q == '\0') break;
                readFaceVertex(q, vi[n], ti[n], ni[n]);
                n++;
            }
            std::vector<float>& dst = out.vertices[cur];
            for (int k = 1; k + 1 < n; k++)          // triangle fan (this model is all triangles)
            {
                int idx[3] = { 0, k, k + 1 };
                for (int j = 0; j < 3; j++)
                {
                    int a = idx[j];
                    int pi = fixIndex(vi[a], pos.size());
                    int ui = fixIndex(ti[a], uv.size());
                    int ci = fixIndex(ni[a], nor.size());
                    glm::vec3 P = pi >= 0 ? pos[pi] : glm::vec3(0.0f);
                    glm::vec3 N = ci >= 0 ? nor[ci] : glm::vec3(0.0f, 1.0f, 0.0f);
                    glm::vec2 T = ui >= 0 ? uv[ui] : glm::vec2(0.0f);
                    dst.insert(dst.end(), { P.x, P.y, P.z, N.x, N.y, N.z, T.x, T.y });
                }
            }
        }
        else if (std::strncmp(line, "usemtl ", 7) == 0)
        {
            cur = getMat(readRest(line + 7));
        }
        else if (std::strncmp(line, "mtllib ", 7) == 0)
        {
            out.mtlFile = readRest(line + 7);
        }

        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return true;
}

inline std::map<std::string, MtlInfo> parseMtl(const std::string& path)
{
    using namespace modelDetail;
    std::map<std::string, MtlInfo> result;
    std::string text;
    if (!readFile(path, text))
    {
        std::cout << "Cannot open mtl: " << path << std::endl;
        return result;
    }
    MtlInfo* cur = nullptr;
    const char* p = text.c_str();
    while (*p)
    {
        const char* line = skipSpaces(p);
        if (std::strncmp(line, "newmtl ", 7) == 0)
        {
            cur = &result[readRest(line + 7)];
        }
        else if (cur && line[0] == 'K' && line[1] == 'd' && line[2] == ' ')
        {
            char* e;
            float r = std::strtof(line + 3, &e);
            float g = std::strtof(e, &e);
            float b = std::strtof(e, &e);
            cur->kd = glm::vec3(r, g, b);
        }
        else if (cur && std::strncmp(line, "map_Kd ", 7) == 0)
        {
            cur->mapKd = baseName(readRest(line + 7));
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return result;
}

class Model
{
public:
    struct Part
    {
        std::string material;
        unsigned int vao = 0;
        int vertexCount = 0;
        unsigned int texture = 0;
        glm::vec3 emissive = glm::vec3(0.0f);
    };

    std::vector<Part> parts;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);

    glm::vec3 size() const { return boundsMax - boundsMin; }

    // objPath example: "assets/models/christmas_tree/obj.obj"
    // the mtl and textures are expected in the same folder as the obj
    bool load(const std::string& objPath)
    {
        std::string dir;
        size_t slash = objPath.find_last_of("/\\");
        if (slash != std::string::npos) dir = objPath.substr(0, slash + 1);

        ObjData data;
        if (!parseObj(objPath, data)) return false;
        boundsMin = data.boundsMin;
        boundsMax = data.boundsMax;

        std::map<std::string, MtlInfo> mtl;
        if (!data.mtlFile.empty()) mtl = parseMtl(dir + data.mtlFile);

        std::map<std::string, unsigned int> textureCache;

        for (size_t i = 0; i < data.materialNames.size(); i++)
        {
            std::vector<float>& v = data.vertices[i];
            if (v.empty()) continue;

            Part part;
            part.material = data.materialNames[i];
            part.vertexCount = (int)(v.size() / 8);

            MtlInfo info;
            auto it = mtl.find(part.material);
            if (it != mtl.end()) info = it->second;

            if (!info.mapKd.empty())
            {
                auto c = textureCache.find(info.mapKd);
                if (c == textureCache.end())
                    c = textureCache.emplace(info.mapKd, loadTextureClamp(dir + info.mapKd)).first;
                part.texture = c->second;
            }
            else
            {
                part.texture = makeColor(info.kd);
            }

            part.vao = upload(v);
            std::vector<float>().swap(v);   // free CPU memory
            parts.push_back(part);
        }
        std::cout << "Loaded model " << objPath << " (" << parts.size() << " materials)" << std::endl;
        return !parts.empty();
    }

    // add emissive glow to one material (e.g. the star on top)
    void setEmissive(const std::string& material, const glm::vec3& e)
    {
        for (Part& p : parts)
            if (p.material == material) p.emissive = e;
    }

    void draw(const Shader& shader, const glm::mat4& model) const
    {
        shader.setMat4("model", model);
        shader.setVec2("uvScale", glm::vec2(1.0f));
        shader.setVec2("uvOffset", glm::vec2(0.0f));
        for (const Part& p : parts)
        {
            shader.setVec3("emissive", p.emissive);
            glBindTexture(GL_TEXTURE_2D, p.texture);
            glBindVertexArray(p.vao);
            glDrawArrays(GL_TRIANGLES, 0, p.vertexCount);
        }
    }

private:
    static unsigned int upload(const std::vector<float>& v)
    {
        unsigned int vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        return vao;
    }

    // gradient ramp textures use CLAMP so UVs outside 0..1 do not wrap
    static unsigned int loadTextureClamp(const std::string& path)
    {
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        int w, h, ch;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 3);
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

    // materials without a texture: make a 1x1 texture from the Kd color
    static unsigned int makeColor(const glm::vec3& kd)
    {
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glm::vec3 c = glm::clamp(kd, 0.0f, 1.0f) * 255.0f;
        unsigned char px[3] = { (unsigned char)(c.r + 0.5f), (unsigned char)(c.g + 0.5f), (unsigned char)(c.b + 0.5f) };
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
        return tex;
    }
};
