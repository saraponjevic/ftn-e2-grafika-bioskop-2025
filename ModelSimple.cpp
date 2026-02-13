// ModelSimple.cpp
#include "ModelSimple.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "Util.h"

struct VertexSimple {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

static std::string getDir(const std::string& path) {
    size_t p = path.find_last_of("/\\");
    return (p == std::string::npos) ? "." : path.substr(0, p);
}

static GLuint tryLoadTextureInDir(const std::string& dir, const std::string& relFromMtl = "")
{
    // 1) ako Assimp vrati putanju (iz .mtl), probaj prvo to
    if (!relFromMtl.empty()) {
        std::string full = dir + "/" + relFromMtl;
        GLuint t = loadImageToTexture(full.c_str());
        if (t) return t;
    }

    // 2) fallback: probaj najèešæa imena fajlova
    const char* candidates[] = {
        "model_texture.png", "model_texture.jpg", "model_texture.jpeg",
        "diffuse.png", "diffuse.jpg",
        "albedo.png", "albedo.jpg",
        "texture.png", "texture.jpg",
        "tex.png", "tex.jpg"
    };

    for (auto c : candidates) {
        std::string full = dir + "/" + c;
        GLuint t = loadImageToTexture(full.c_str());
        if (t) return t;
    }

    return 0;
}

void ModelSimple::clear()
{
    for (auto& m : meshes) {
        if (m.diffuseTex) glDeleteTextures(1, &m.diffuseTex);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        m = {};
    }
    meshes.clear();
}

bool ModelSimple::load(const std::string& path)
{
    clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FlipUVs
    );

    if (!scene || !scene->mRootNode) {
        std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << "\n";
        return false;
    }

    std::string dir = getDir(path);

    for (unsigned int mi = 0; mi < scene->mNumMeshes; mi++)
    {
        aiMesh* mesh = scene->mMeshes[mi];
        if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0) continue;

        std::vector<VertexSimple> vertices;
        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            VertexSimple v{};
            v.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            v.normal = mesh->HasNormals()
                ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                : glm::vec3(0, 1, 0);

            if (mesh->HasTextureCoords(0)) {
                v.uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y);
            }
            else {
                v.uv = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(v);
        }

        std::vector<unsigned int> indices;
        indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        MeshGL out{};
        out.indexCount = (GLsizei)indices.size();

        glGenVertexArrays(1, &out.VAO);
        glGenBuffers(1, &out.VBO);
        glGenBuffers(1, &out.EBO);

        glBindVertexArray(out.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, out.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexSimple), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)offsetof(VertexSimple, pos));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)offsetof(VertexSimple, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)offsetof(VertexSimple, uv));

        glBindVertexArray(0);

        // --- TEXTURE: probaj iz materijala (.mtl) pa fallback ---
        std::string texRel = "";
        if (scene->mMaterials && mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            if (mat && mat->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
                mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
            {
                texRel = texPath.C_Str(); // npr "model_texture.png"
            }
        }

        out.diffuseTex = tryLoadTextureInDir(dir, texRel);
        if (!out.diffuseTex) {
            std::cout << "[WARN] Texture not found for: " << path << " (mesh " << mi << ")\n";
        }

        meshes.push_back(out);
    }

    return !meshes.empty();
}

void ModelSimple::Draw() const
{
    for (const auto& m : meshes) {
        glBindVertexArray(m.VAO);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
