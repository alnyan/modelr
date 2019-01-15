#include "wavefront.h"
#include "../res/assets.h"
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

bool Wavefront::loadObj(Model *model, std::list<std::string> *reqMaterials, MeshBuilder *mesh, const std::string &path) {
    // TODO: material loading
    if (reqMaterials) {
        reqMaterials->clear();
    }

    std::ifstream file(Assets::getModelPath(path));
    std::vector<glm::vec3> vertexData;
    std::vector<glm::vec2> texCoordData;
    std::vector<glm::vec3> normalData;
    std::string line;
    glm::vec3 v;

    if (!file) {
        std::cout << "Failed to open " << path << std::endl;
        return false;
    }

    std::cout << "Load model " << path << std::endl;

    // Begin new mesh
    model->begin = mesh->beginShape();

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
            // Face specification
            const char *data = l + 2;
            std::list<WfVertexAttribIndex> is;

            if (!wfParseFace(data, is)) {
                std::cerr << "Invalid face:" << std::endl << line << std::endl;
                return false;
            }

            int vc = is.size();

            switch (vc) {
            case 3:
                if (!wfTriangle(mesh, vertexData, texCoordData, normalData, is)) {
                    std::cerr << "Invalid triangle:" << std::endl << line << std::endl;
                    return false;
                }
                break;
            case 4:
                if (!wfQuad(mesh, vertexData, texCoordData, normalData, is)) {
                    std::cerr << "Invalid quad:" << std::endl << line << std::endl;
                    return false;
                }
                break;
            default:
                std::cerr << "Unsupported vertex count per face: " << vc << std::endl;
                return false;
            }
        }
    }

    model->size = mesh->endShape();

    return true;
}
