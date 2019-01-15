#include "wavefront.h"
#include "../res/assets.h"
#include "../resources.h"
#include <iostream>
#include <fstream>
#include <string>
#include <list>

using WfVertexAttribIndex = glm::vec<3, int>;

static bool wfParseIndex(const char *l, WfVertexAttribIndex &i) {
    i = glm::vec<3, int>(-1, -1, -1);
    const char *f = strchrnul(l, '/');
    i.x = atoi(std::string(l, f).c_str()) - 1; // Vertex coord index

    if (*f == 0) {
        return true;
    }

    l = f + 1;

    if (*l == '/') {
        // Cases like "1//3"
        ++l;
    } else {
        // Cases like "1/2/3" or "1/2"
        f = strchrnul(l, '/');
        i.y = atoi(std::string(l, f).c_str()) - 1;

        if (*f == 0) {
            // No normal
            return true;
        }

        l = f + 1;
    }

    i.z = atoi(l) - 1;

    return true;
}

static bool wfParseFace(const char *l, std::list<WfVertexAttribIndex> &is) {
    const char *e;
    WfVertexAttribIndex i;

    while (true) {
        while (*l == ' ') {
            ++l;
        }

        e = strchrnul(l, ' ');
        std::string indexStr(l, e);

        if (!wfParseIndex(indexStr.c_str(), i)) {
            return false;
        }
        is.push_back(i);

        if (*e == 0) {
            break;
        }

        l = e + 1;
    }

    return true;
}

static bool wfTriangle(MeshBuilder *mesh,
                       const std::vector<glm::vec3> &vertexData,
                       const std::vector<glm::vec2> &texCoordData,
                       const std::vector<glm::vec3> &normalData,
                       const std::list<WfVertexAttribIndex> &is) {
    int j = 0;
    for (const auto &i: is) {
        // Normals and vertices are mandatory
        if (i.x < 0 || i.z < 0 || i.x >= vertexData.size() || i.z >= normalData.size()) {
            return false;
        }

        if (i.y < 0 || i.y >= texCoordData.size()) {
            // 0 0, 1 0, 1 1
            mesh->texCoord({ j != 0, j == 2 });
        } else {
            mesh->texCoord(texCoordData[i.y]);
        }

        mesh->normal(normalData[i.z]);
        mesh->vertex(vertexData[i.x]);

        ++j;
    }

    return true;
}

static bool wfQuad(MeshBuilder *mesh,
                   const std::vector<glm::vec3> &vertexData,
                   const std::vector<glm::vec2> &texCoordData,
                   const std::vector<glm::vec3> &normalData,
                   const std::list<WfVertexAttribIndex> &is) {
    auto it = is.begin();

    for (int j = 0; j < 3; ++j) {
        auto i = *it++;

        // Normals and vertices are mandatory
        if (i.x < 0 || i.z < 0 || i.x >= vertexData.size() || i.z >= normalData.size()) {
            return false;
        }

        if (i.y < 0 || i.y >= texCoordData.size()) {
            // 0 0, 1 0, 1 1
            mesh->texCoord({ j != 0, j == 2 });
        } else {
            mesh->texCoord(texCoordData[i.y]);
        }

        mesh->normal(normalData[i.z]);
        mesh->vertex(vertexData[i.x]);
    }

    it = is.begin();
    ++it;
    ++it;

    for (int j = 0; j < 2; ++j) {
        auto i = *it++;

        // Normals and vertices are mandatory
        if (i.x < 0 || i.z < 0 || i.x >= vertexData.size() || i.z >= normalData.size()) {
            return false;
        }

        if (i.y < 0 || i.y >= texCoordData.size()) {
            // 1 1, 0 1
            mesh->texCoord({ j == 0, true });
        } else {
            mesh->texCoord(texCoordData[i.y]);
        }

        mesh->normal(normalData[i.z]);
        mesh->vertex(vertexData[i.x]);
    }

    auto i = *is.begin();

    // Normals and vertices are mandatory
    if (i.x < 0 || i.z < 0 || i.x >= vertexData.size() || i.z >= normalData.size()) {
        return false;
    }

    if (i.y < 0 || i.y >= texCoordData.size()) {
        mesh->texCoord({ 0, 0 });
    } else {
        mesh->texCoord(texCoordData[i.y]);
    }

    mesh->normal(normalData[i.z]);
    mesh->vertex(vertexData[i.x]);

    return true;
}

int Wavefront::loadMtl(const std::string &path) {
    std::ifstream file(Assets::getModelPath(path));
    std::string line, mtlName;

    if (!file) {
        std::cout << "Failed to open " << path << std::endl;
        return -1;
    }

    std::cout << "Load material " << path << std::endl;

    MaterialUniformData *mtl = nullptr;
    std::string materialName;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const char *l = line.c_str();

        std::string fun(l, strchrnul(l, ' '));

        if (fun == "newmtl") {
            char name[1024];
            sscanf(l, "newmtl %s", name);

            // Begin new material
            if (mtl) {
                std::cout << "#endmtl " << path << ":" << materialName << std::endl;
            }

            materialName = name;
            if (createMaterialObject(path + ":" + materialName, &mtl) < 0) {
                std::cout << "Failed to create new material \"" << path << ":" << materialName << "\"" << std::endl;
                return -1;
            }

            std::cout << "#newmtl " << path << ":" << materialName << std::endl;
        } else if (fun == "map_Kd") {
            assert(mtl);

            char name[1024];
            sscanf(l, "map_Kd %s", name);

            // Load new texture
            int map_Kd_idx = loadTexture(Assets::getTexturePath(name));

            if (map_Kd_idx < 0) {
                std::cout << "Failed to load diffuse map: " << name << std::endl;
            } else {
                std::cout << "#map_Kd " << materialName << " : " << name << " : " << map_Kd_idx << std::endl;
                mtl->m_maps[0] = map_Kd_idx;
            }
        } else if (fun == "map_Bump") {
            assert(mtl);

            char name[1024];
            sscanf(l, "map_Bump %s", name);

            int map_Bump_idx = loadTexture(Assets::getTexturePath(name));

            if (map_Bump_idx < 0) {
                std::cout << "Failed to load bump map: " << name << std::endl;
            } else {
                std::cout << "#map_Bump " << materialName << " : " << name << " : " << map_Bump_idx << std::endl;
                mtl->m_maps[1] = map_Bump_idx;
            }
        } else if (fun == "Ks") {
            assert(mtl);

            sscanf(l, "Ks %f %f %f", &mtl->m_Ks.x, &mtl->m_Ks.y, &mtl->m_Ks.z);
        }
    }

    if (mtl) {
        std::cout << "#endmtl " << path << ":" << materialName << std::endl;
    }

    return 0;
}

int Wavefront::loadObj(MeshBuilder *mesh, const std::string &path) {
    // TODO: material loading
    std::ifstream file(Assets::getModelPath(path));
    std::string mtllib, partName;
    std::vector<glm::vec3> vertexData;
    std::vector<glm::vec2> texCoordData;
    std::vector<glm::vec3> normalData;
    std::string line;
    glm::vec3 v;

    if (!file) {
        std::cout << "Failed to open " << path << std::endl;
        return -1;
    }

    std::cout << "Load model " << path << std::endl;

    Model *model;

    if (createModelObject(path, &model) < 0 || !model) {
        return -1;
    }

    Part *part = nullptr;
    int partIndex = 0;

    // Begin new mesh
    //model->begin = mesh->beginShape();

    // For debugging
    std::cout << "#model " << path << std::endl;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const char *l = line.c_str();

        std::string fun(l, strchrnul(l, ' '));

        if (fun == "v") {
            // Vertex
            sscanf(l, "v %f %f %f", &v.x, &v.y, &v.z);
            vertexData.push_back(v);
        } else if (fun == "vn") {
            // Normal
            sscanf(l, "vn %f %f %f", &v.x, &v.y, &v.z);
            normalData.push_back(v);
        } else if (fun == "vt") {
            // TexCoord
            sscanf(l, "vt %f %f", &v.x, &v.y);
            texCoordData.push_back({ v.x, v.y });
        } else if (fun == "f") {
            if (!part) {
                std::cerr << "Face belongs to no object/part" << std::endl;
                return -1;
            }

            // Face specification
            const char *data = l + 2;
            std::list<WfVertexAttribIndex> is;

            if (!wfParseFace(data, is)) {
                std::cerr << "Invalid face:" << std::endl << line << std::endl;
                return -1;
            }

            int vc = is.size();

            switch (vc) {
            case 3:
                if (!wfTriangle(mesh, vertexData, texCoordData, normalData, is)) {
                    std::cerr << "Invalid triangle:" << std::endl << line << std::endl;
                    return -1;
                }
                break;
            case 4:
                if (!wfQuad(mesh, vertexData, texCoordData, normalData, is)) {
                    std::cerr << "Invalid quad:" << std::endl << line << std::endl;
                    return -1;
                }
                break;
            default:
                std::cerr << "Unsupported vertex count per face: " << vc << std::endl;
                return -1;
            }
        } else if (fun == "mtllib") {
            char name[1024];
            memset(name, 0, sizeof(name));
            sscanf(l, "mtllib %s", name);

            // Required material file
            mtllib = name;
            // Try to load material library
            if (loadMtl(name) != 0) {
                std::cerr << "Material loading failed" << std::endl;
                return -1;
            }
        } else if (fun == "usemtl") {
            assert(part);

            if (part->materialIndex != -1) {
                std::cerr << "Part already has material assigned" << std::endl;
                return -1;
            }

            char name[1024];
            memset(name, 0, sizeof(name));
            sscanf(l, "usemtl %s", name);

            // Required material file:name
            std::cout << "#require " << mtllib << ":" << name << std::endl;

            int mtlIdx = getMaterialIndex(mtllib + ":" + std::string(name));

            if (mtlIdx < 0) {
                std::cout << "Unknown material: " << mtllib << ":" << name << std::endl;
            } else {
                part->materialIndex = mtlIdx;
            }
        } else if (fun == "o") {
            // Object/part
            char name[1024];
            memset(name, 0, sizeof(name));
            sscanf(l, "o %s", name);

            if (part) {
                part->size = mesh->endShape();
                std::cout << "#endpart " << partName << ": " << part->size << std::endl;
            }

            // Create new part
            model->parts.push_back({});
            part = &model->parts[partIndex++];
            part->begin  = mesh->beginShape();
            part->materialIndex = -1;

            std::cout << "#part " << name << ": " << part->begin << std::endl;
            partName = name;
        }
    }

    if (part) {
        part->size = mesh->endShape();
        std::cout << "#endpart " << partName << ": " << part->size << std::endl;
    }

    return 0;
}
