#include "resources.h"
#include "render/common.h"
#include "render/wavefront.h"
#include "render/model.h"
#include "res/lodepng.h"
#include <iostream>
#include <vector>
#include <list>
#include <map>

static GLuint64 s_textureHandles[S_TEXTURE_COUNT * 2];
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

int addTextureHandle(int index, GLuint64 handle) {
    std::cout << "res:textureHandles[" << index << "] = " << handle << std::endl;
    s_textureHandles[textureIndex(index)] = handle;
    return 0;
}

int genTextureHandle(int index, GLuint texID) {
    GLuint64 handle = glGetTextureHandleARB(texID);
    if (glGetError()) {
        std::cerr << "Failed to generate texture handle" << std::endl;
        return -1;
    }
    glMakeTextureHandleResidentARB(handle);

    std::cout << "res:textureHandles[" << index << "] = " << handle << std::endl;
    s_textureHandles[textureIndex(index)] = handle;
    return 0;
}

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

void uploadTextureHandles(GLuint buffer) {
    std::cout << s_lastTextureIndex << " material-used texture handles" << std::endl;
    for (int i = 0; i < s_lastTextureIndex; ++i) {
        genTextureHandle(i, s_textureIDs[i]);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_textureHandles), s_textureHandles, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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

        std::cout << "res:createMtl " << name<< " = " << idx << std::endl;
        mat->m_maps[1] = S_TEXTURE_UNDEFINED;
        mat->m_maps[2] = S_TEXTURE_UNDEFINED;
        mat->m_maps[0] = S_TEXTURE_UNDEFINED;
        mat->m_Ks = glm::vec4(1, 1, 1, 100);
        mat->m_Ka = glm::vec4(0.2, 0.2, 0.2, 0);

        return idx;
    }
}

int getMaterialIndex(const std::string &name) {
    auto it = s_materialBinding.find(name);
    if (it == s_materialBinding.end()) {
        return -1;
    }

    return it->second;
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
    //(*mod)->materialIndex = -1;
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

int getModelIndex(const std::string &name) {
    auto it = s_modelBindings.find(name);
    if (it == s_modelBindings.end()) {
        return -1;
    }
    return it->second;
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

    res = Wavefront::loadObj(mesh, file); // TODO: material linking

    return res;
}
