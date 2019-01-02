#include "model.h"
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <list>

//

using ObjVertexCoords = glm::vec<3, float>;
using ObjVertexIndex = std::list<glm::vec<3, int>>;
using ObjTexCoords = glm::vec<2, float>;

//

MeshBuilder::MeshBuilder() {
    glCreateBuffers(1, &vertexBufferID);
    glCreateBuffers(1, &texCoordBufferID);
    glCreateBuffers(1, &normalBufferID);
    m_u = m_v = 0;
    m_nx = m_ny = m_nz = 0;
}

void MeshBuilder::uploadMesh() {
    if (vertexCount) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * vertexCount, &m_vertexData[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * vertexCount, &m_texCoordData[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * vertexCount, &m_normalData[0], GL_STATIC_DRAW);
    }
}

void MeshBuilder::texCoord(GLfloat u, GLfloat v) {
    m_u = u;
    m_v = v;
}

void MeshBuilder::normal(GLfloat nx, GLfloat ny, GLfloat nz) {
    m_nx = nx;
    m_ny = ny;
    m_nz = nz;
}

void MeshBuilder::vertex(GLfloat x, GLfloat y, GLfloat z) {
    m_texCoordData.push_back(m_u);
    m_texCoordData.push_back(m_v);
    m_normalData.push_back(m_nx);
    m_normalData.push_back(m_ny);
    m_normalData.push_back(m_nz);
    m_vertexData.push_back(x);
    m_vertexData.push_back(y);
    m_vertexData.push_back(z);
    m_u = m_v = 0;
    m_nx = m_ny = m_nz = 0;
    ++vertexCount;
}

//

Model::Model(size_t ns, GLuint vid, GLuint tid, GLuint nid, Material *mat):
    m_material{mat},
    m_vertexBufferID{vid},
    m_texCoordBufferID{tid},
    m_normalBufferID{nid},
    m_vertexCount{ns} {}

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

static bool objFaceTriangle(const std::vector<ObjVertexCoords> &vs,
                        const std::vector<ObjTexCoords> &ts,
                        const std::vector<ObjVertexCoords> &ns,
                        MeshBuilder &mesh,
                        const ObjVertexIndex &idx) {
    auto it = idx.begin();
    for (int i = 0; i < 3; ++i) {
        auto p = *it++;
        if (p.x < 0 || p.x >= vs.size() || p.z < 0 || p.z >= ns.size()) {
            return false;
        }

        mesh.normal(ns[p.z].x, ns[p.z].y, ns[p.z].z);

        if (p.y < 0 || p.y >= ts.size()) {
            mesh.texCoord(i != 0, i == 2);
        } else {
            mesh.texCoord(ts[p.y].x, ts[p.y].y);
        }

        mesh.vertex(vs[p.x].x, vs[p.x].y, vs[p.x].z);
    }

    return true;
}

static bool objFaceQuad(const std::vector<ObjVertexCoords> &vs,
                        const std::vector<ObjTexCoords> &ts,
                        const std::vector<ObjVertexCoords> &ns,
                        MeshBuilder &mesh,
                        const ObjVertexIndex &idx) {
    auto i0 = idx.begin();
    ObjVertexIndex t0, t1;

    t0.push_back(*i0);      // 0: 0
    t1.push_back(*i0++);    // 1: 0
    t0.push_back(*i0++);    // 0: 0 1
    t1.push_back(*i0);      // 1: 0 2
    t0.push_back(*i0++);    // 0: 0 1 2
    t1.push_back(*i0);      // 1: 0 2 3

    return
        objFaceTriangle(vs, ts, ns, mesh, t0) &&
        objFaceTriangle(vs, ts, ns, mesh, t1);
}

Model *Model::loadObj(const std::string &path) {
    std::ifstream file(path);
    Material *mat = nullptr;

    if (!file) {
        return nullptr;
    }

    std::string line;
    std::vector<ObjVertexCoords> vertices;
    std::vector<ObjVertexCoords> normals;
    std::vector<ObjTexCoords> texCoords;
    MeshBuilder mesh;

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
                delete mat;
                return nullptr;
            }

            switch (nv) {
                case 4:
                    // Quad
                    if (!objFaceQuad(vertices, texCoords, normals, mesh, i)) {
                        std::cerr << "Invalid quad:" << std::endl << line << std::endl;
                        delete mat;
                        return nullptr;
                    }
                    break;
                case 3:
                    if (!objFaceTriangle(vertices, texCoords, normals, mesh, i)) {
                        std::cerr << "Invalid triangle:" << std::endl << line << std::endl;
                        delete mat;
                        return nullptr;
                    }
                    break;
                default:
                    std::cerr << "Unsupported face type: " << nv << " vertices" << std::endl;
                    delete mat;
                    return nullptr;
            }
        } else if (!strncmp(line.c_str(), "mtllib ", 7)) {
            char path[1024];
            sscanf(line.c_str(), "mtllib %s", path);

            mat = Material::loadMtl(path);

            if (!mat) {
                std::cerr << "Model: failed to load material" << path << std::endl;
                return nullptr;
            }
        }
    }

    mesh.uploadMesh();

    return new Model(mesh.vertexCount, mesh.vertexBufferID, mesh.texCoordBufferID, mesh.normalBufferID, mat);
}
