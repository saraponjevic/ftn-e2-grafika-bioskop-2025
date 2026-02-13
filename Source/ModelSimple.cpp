#include "ModelSimple.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

struct VertexSimple {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

void ModelSimple::clear() {
    for (auto& m : meshes) {
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
    }
    meshes.clear();
}

bool ModelSimple::load(const std::string& path) {
    clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode) {
        std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << "\n";
        return false;
    }

    for (unsigned int mi = 0; mi < scene->mNumMeshes; mi++) {
        aiMesh* mesh = scene->mMeshes[mi];

        std::vector<VertexSimple> vertices;
        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            VertexSimple v{};
            v.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            if (mesh->HasNormals())
                v.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            else
                v.normal = glm::vec3(0, 1, 0);

            if (mesh->mTextureCoords[0])
                v.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                v.uv = glm::vec2(0, 0);

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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)offsetof(VertexSimple, normal));

        // UV postoji (location 2), ali phong ga ne koristi sad
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), (void*)offsetof(VertexSimple, uv));

        glBindVertexArray(0);

        meshes.push_back(out);
    }

    std::cout << "Loaded model: " << path << " meshes=" << meshes.size() << "\n";
    return true;
}

void ModelSimple::Draw() const {
    for (const auto& m : meshes) {
        glBindVertexArray(m.VAO);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}
