// ModelSimple.h
#pragma once
#include <string>
#include <vector>
#include <GL/glew.h>

struct MeshGL {
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;
    GLuint diffuseTex = 0;
};

class ModelSimple {
public:
    ModelSimple() = default;
    ~ModelSimple() { clear(); }

    bool load(const std::string& path);
    void Draw() const;
    void clear();

    GLuint getTexture() const {
        return meshes.empty() ? 0u : meshes[0].diffuseTex;
    }

private:
    std::vector<MeshGL> meshes;
};
