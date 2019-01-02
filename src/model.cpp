#include "model.h"
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <list>

//

using ObjVertexCoords = glm::vec<3, float>;
using ObjVertexIndex = std::list<glm::vec<3, int>>;
using ObjTexCoords = glm::vec<2, float>;

//

Model::Model(const float *vertices, const float *texCoords, const float *normals, Material *mat, size_t count): m_material{mat} {
    glCreateBuffers(1, &m_vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * count, vertices, GL_STATIC_DRAW);
    m_vertexCount = count;
    if (normals) {
        glCreateBuffers(1, &m_normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * count, normals, GL_STATIC_DRAW);
    }
    if (texCoords) {
        glCreateBuffers(1, &m_texCoordBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * count, texCoords, GL_STATIC_DRAW);
    }
}

Model::~Model() {
    if (m_vertexBufferID) {
        glDeleteBuffers(1, &m_vertexBufferID);
    }
    if (m_normalBufferID) {
        glDeleteBuffers(1, &m_normalBufferID);
    }
    if (m_texCoordBufferID) {
        glDeleteBuffers(1, &m_texCoordBufferID);
    }
    delete m_material;
}

void Model::render(Shader *shader) {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    if (m_normalBufferID) {
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, m_normalBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);
    }
    if (m_texCoordBufferID) {
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBufferID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }
    if (m_material) {
        m_material->apply(shader);
    }
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    if (m_texCoordBufferID) {
        glDisableVertexAttribArray(2);
    }
    if (m_normalBufferID) {
        glDisableVertexAttribArray(1);
    }
    glDisableVertexAttribArray(0);
}

static bool objParseIndex(const char *index, int &v, int &t, int &n) {
    v = n = t = -1;

    const char *e = strchr(index, '/');
    if (!e) {
        return false;
    }
    std::string ns(index, e);
    v = atoi(ns.c_str()) - 1;

    index = e + 1;
    if (*index == 0 || *index == ' ') {
        return true;
    }

    if (*index != '/') {
        // Found texture coord
        e = strchr(index, '/');
        if (!e) {
            e = strchrnul(index, ' ');

            ns = std::string(index, e);
            t = atoi(ns.c_str()) - 1;

            return true;
        }

        ns = std::string(index, e);
        t = atoi(ns.c_str()) - 1;
        index = e + 1;

        if (*index == 0 || *index == ' ') {
            return true;
        }
    } else {
        ++index;
    }

    e = strchrnul(index, ' ');
    ns = std::string(index, e);

    n = atoi(ns.c_str()) - 1;
    return true;
}

static const char *skipSpaces(const char *line) {
    while (*line == ' ') {
        ++line;
    }
    return line;
}


static int objParseFace(const char *line, ObjVertexIndex &idx) {
    glm::vec<3, int> item;
    const char *s;
    int n = 0;

    while (line) {
        line = skipSpaces(line);
        if (!objParseIndex(line, item.x, item.y, item.z)) {
            return -1;
        }

        idx.push_back(item);
        ++n;

        line = strchr(line, ' ');
    }

    return n;
}

template<typename T> static bool objFaceTriangle(const std::vector<ObjVertexCoords> &vs,
                        const std::vector<ObjTexCoords> &ts,
                        const std::vector<ObjVertexCoords> &ns,
                        std::vector<float> &vertices,
                        std::vector<float> &texCoords,
                        std::vector<float> &normals,
                        const T &idx) {
    int v = 0;
    for (const auto &i: idx) {
        if (i.x < 0 || i.x >= vs.size()) {
            return false;
        }

        if (i.z < 0 || i.z >= ns.size()) {
            return false;
        }

        vertices.push_back(vs[i.x].x);
        vertices.push_back(vs[i.x].y);
        vertices.push_back(vs[i.x].z);

        if (i.y < 0 || i.y >= ts.size()) {
            texCoords.push_back(v != 0);
            texCoords.push_back(v == 1);
        } else {
            texCoords.push_back(ts[i.y].x);
            texCoords.push_back(ts[i.y].y);
        }

        normals.push_back(ns[i.z].x);
        normals.push_back(ns[i.z].y);
        normals.push_back(ns[i.z].z);

        ++v;
    }

    return true;
}

static bool objFaceQuad(const std::vector<ObjVertexCoords> &vs,
                        const std::vector<ObjTexCoords> &ts,
                        const std::vector<ObjVertexCoords> &ns,
                        std::vector<float> &vertices,
                        std::vector<float> &texCoords,
                        std::vector<float> &normals,
                        const ObjVertexIndex &idx) {
    auto i0 = idx.begin();
    auto i1 = idx.begin();
    ++i1;
    ++i1;

    for (int _ = 0; _ < 3; ++_) {
        auto i = *i0++;

        if (i.x < 0 || i.x >= vs.size()) {
            return false;
        }

        if (i.z < 0 || i.z >= ns.size()) {
            return false;
        }

        vertices.push_back(vs[i.x].x);
        vertices.push_back(vs[i.x].y);
        vertices.push_back(vs[i.x].z);
        normals.push_back(ns[i.z].x);
        normals.push_back(ns[i.z].y);
        normals.push_back(ns[i.z].z);
    }

    for (int _ = 0; _ < 2; ++_) {
        auto i = *i1++;

        if (i.x < 0 || i.x >= vs.size()) {
            return false;
        }

        if (i.z < 0 || i.z >= ns.size()) {
            return false;
        }

        vertices.push_back(vs[i.x].x);
        vertices.push_back(vs[i.x].y);
        vertices.push_back(vs[i.x].z);
        normals.push_back(ns[i.z].x);
        normals.push_back(ns[i.z].y);
        normals.push_back(ns[i.z].z);
    }

    auto i = *(idx.begin());

    if (i.x < 0 || i.x >= vs.size()) {
        return false;
    }

    if (i.z < 0 || i.z >= ns.size()) {
        return false;
    }

    vertices.push_back(vs[i.x].x);
    vertices.push_back(vs[i.x].y);
    vertices.push_back(vs[i.x].z);
    normals.push_back(ns[i.z].x);
    normals.push_back(ns[i.z].y);
    normals.push_back(ns[i.z].z);

    texCoords.push_back(0);
    texCoords.push_back(0);
    texCoords.push_back(1);
    texCoords.push_back(0);
    texCoords.push_back(1);
    texCoords.push_back(1);
    texCoords.push_back(1);
    texCoords.push_back(1);
    texCoords.push_back(0);
    texCoords.push_back(1);
    texCoords.push_back(0);
    texCoords.push_back(0);

    return true;
}

Model *Model::loadObj(const std::string &path) {
    std::ifstream file(path);

    if (!file) {
        return nullptr;
    }

    std::string line;
    std::vector<ObjVertexCoords> vertices;
    std::vector<ObjVertexCoords> normals;
    std::vector<ObjTexCoords> texCoords;
    std::vector<float> faceVertices;
    std::vector<float> faceTexCoords;
    std::vector<float> faceNormals;
    size_t faceCount = 0;

    while (std::getline(file, line)) {
        if (!strncmp(line.c_str(), "v ", 2)) {
            ObjVertexCoords v;
            sscanf(line.c_str(), "v %f %f %f", &v.x, &v.y, &v.z);
            vertices.push_back(v);
        } else if (!strncmp(line.c_str(), "vn ", 3)) {
            ObjVertexCoords v;
            sscanf(line.c_str(), "vn %f %f %f", &v.x, &v.y, &v.z);
            normals.push_back(v);
        } else if (!strncmp(line.c_str(), "vt ", 3)) {
            ObjTexCoords t;
            sscanf(line.c_str(), "vt %f %f", &t.x, &t.y);
            texCoords.push_back(t);
        } else if (!strncmp(line.c_str(), "f ", 2)) {
            ObjVertexIndex i;
            size_t siz = vertices.size();
            size_t norms = normals.size();

            const char *s = skipSpaces(line.c_str() + 2);
            int nv = objParseFace(s, i);

            if (nv == -1) {
                std::cerr << "Failed to parse face:" << std::endl << line << std::endl;
                return nullptr;
            }

            switch (nv) {
                case 4:
                    // Quad
                    if (!objFaceQuad(vertices, texCoords, normals, faceVertices, faceTexCoords, faceNormals, i)) {
                        std::cerr << "Invalid quad:" << std::endl << line << std::endl;
                        return nullptr;
                    }
                    faceCount += 2;
                    break;
                case 3:
                    if (!objFaceTriangle(vertices, texCoords, normals, faceVertices, faceTexCoords, faceNormals, i)) {
                        std::cerr << "Invalid triangle:" << std::endl << line << std::endl;
                        return nullptr;
                    }
                    faceCount += 1;
                    break;
                default:
                    std::cerr << "Unsupported face type: " << nv << " vertices" << std::endl;
                    return nullptr;
            }
        }
    }

    return new Model(&faceVertices[0], &faceTexCoords[0], &faceNormals[0], nullptr, faceCount * 3);
}
