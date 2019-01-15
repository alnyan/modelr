#include "resources.h"
#include "render/common.h"
#include "render/wavefront.h"
#include "render/model.h"
#include "res/lodepng.h"
#include <iostream>
#include <vector>
#include <list>
#include <map>

static GLuint s_textureIDs[S_TEXTURE_COUNT];
static MaterialUniformData s_materials[S_MATERIAL_COUNT];
static std::vector<Model> s_models;

static int s_lastTextureIndex = 0;
static int s_lastMaterialIndex = 0;
static int s_lastModelIndex = 0;
static std::map<std::string, int> s_materialBinding;
static std::map<std::string, int> s_textureBindings;
static std::map<std::string, int> s_modelBindings;

////

GLuint getTextureID(int index) {
    return s_textureIDs[index];
}

int loadTextureFixed(GLuint id, const std::string &path, GLuint minFilter, GLuint magFilter) {
    unsigned int w, h;
    std::vector<unsigned char> data;

    if (lodepng::decode(data, w, h, path) != 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return -1;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}

int loadTexture(const std::string &path, GLuint minFilter, GLuint magFilter) {
    auto it = s_textureBindings.find(path);
    if (it != s_textureBindings.end()) {
        return it->second;
    } else {
        int idx = s_lastTextureIndex++;
        s_textureBindings[path] = idx;
        if (idx >= S_TEXTURE_MAX) {
            std::cerr << "No free texture IDs left" << std::endl;
            return -1;
        }

        glGenTextures(1, &s_textureIDs[idx]);

        if (loadTextureFixed(s_textureIDs[idx], path, minFilter, magFilter) != 0) {
            return -1;
        }

        return idx;
    }
}

int loadTextureMulti(int *indices, const std::list<std::string> &items) {
    int n = items.size();
    int i = 0;

    for (const auto &path: items) {
        if ((indices[i] = loadTexture(path)) < 0) {
            return -1;
        }
        ++i;
    }

    return 0;
}

int getTextureIndex(const std::string &name) {
    auto it = s_textureBindings.find(name);
    if (it != s_textureBindings.end()) {
        return it->second;
    } else {
        return -1;
    }
}

////

int createMaterialObject(const std::string &name, MaterialUniformData **obj) {
    auto it = s_materialBinding.find(name);
    if (it != s_materialBinding.end()) {
        // Already exists
        return -1;
    } else {
        int idx = s_lastMaterialIndex++;
        if (idx >= S_MATERIAL_COUNT) {
            std::cerr << "Failed to allocate material" << std::endl;
            return -1;
        }

        MaterialUniformData *mat = &s_materials[idx];

        s_materialBinding[name] = idx;

        *obj = mat;
        // Initial setup
        mat->m_maps[1] = S_TEXTURE_UNDEFINED;
        mat->m_maps[2] = S_TEXTURE_UNDEFINED;
        mat->m_maps[0] = S_TEXTURE_UNDEFINED;
        mat->m_Ks = glm::vec4(0.2, 0.2, 0.2, 100);

        return idx;
    }
}

void uploadMaterials(GLuint buffer) {
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_materials), s_materials, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

////

int createModelObject(const std::string &name, Model **mod) {
    auto it = s_modelBindings.find(name);
    if (it != s_modelBindings.end()) {
        // Already exists
        std::cout << "Failed to create \"" << name << "\": already exists" << std::endl;
        return -1;
    }

    s_models.push_back({});
    *mod = &s_models[s_lastModelIndex];
    s_modelBindings[name] = s_lastModelIndex;

    return s_lastModelIndex++;
}

Model *getModelObject(const std::string &name) {
    auto it = s_modelBindings.find(name);
    if (it != s_modelBindings.end()) {
        return &s_models[it->second];
    }
    return nullptr;
}

Model *getModelObject(int index) {
    if (index < 0 || index >= s_models.size()) {
        return nullptr;
    }

    return &s_models[index];
}

int loadModel(MeshBuilder *mesh, const std::string &file) {
    std::cout << "loadModel " << file << std::endl;
    int res;
    Model *mod;

    if (createModelObject(file, &mod) < 0) {
        std::cerr << "Failed to create new model object" << std::endl;
        return -1;
    }

    res = Wavefront::loadObj(mod, nullptr, mesh, file); // TODO: material linking

    std::cout << "Loaded" << std::endl;

    return -!res;
}
