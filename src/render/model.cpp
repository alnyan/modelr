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

MeshBuilder::MeshBuilder(bool hasTangents): m_genTangents{hasTangents} {
    glCreateBuffers(hasTangents ? 5 : 3, buffer.bufferIDs);
    m_u = m_v = 0;
    m_nx = m_ny = m_nz = 0;
    buffer.vertexCount = 0;
}

void MeshBuilder::genTangents() {
    for (int i = 0; i < buffer.vertexCount; i += 3) {
        glm::vec3 v0(m_vertexData[i * 3 + 0], m_vertexData[i * 3 + 1], m_vertexData[i * 3 + 2]);
        glm::vec3 v1(m_vertexData[i * 3 + 3], m_vertexData[i * 3 + 4], m_vertexData[i * 3 + 5]);
        glm::vec3 v2(m_vertexData[i * 3 + 6], m_vertexData[i * 3 + 7], m_vertexData[i * 3 + 8]);
        glm::vec2 t0(m_texCoordData[i * 2 + 0], m_texCoordData[i * 2 + 1]);
        glm::vec2 t1(m_texCoordData[i * 2 + 2], m_texCoordData[i * 2 + 3]);
        glm::vec2 t2(m_texCoordData[i * 2 + 4], m_texCoordData[i * 2 + 5]);

        glm::vec3 dv0 = v1 - v0;
        glm::vec3 dv1 = v2 - v0;

        glm::vec2 dt0 = t1 - t0;
        glm::vec2 dt1 = t2 - t0;

        float r = 1.0f / (dt0.x * dt1.y - dt0.y * dt1.x);

        glm::vec3 tangent = (dv0 * dt1.y - dv1 * dt0.y) * r;
        glm::vec3 bitangent = (dv1 * dt0.x - dv0 * dt1.x) * r;

        m_tangentData.push_back(tangent.x);
        m_tangentData.push_back(tangent.y);
        m_tangentData.push_back(tangent.z);
        m_tangentData.push_back(tangent.x);
        m_tangentData.push_back(tangent.y);
        m_tangentData.push_back(tangent.z);
        m_tangentData.push_back(tangent.x);
        m_tangentData.push_back(tangent.y);
        m_tangentData.push_back(tangent.z);
        m_bitangentData.push_back(bitangent.x);
        m_bitangentData.push_back(bitangent.y);
        m_bitangentData.push_back(bitangent.z);
        m_bitangentData.push_back(bitangent.x);
        m_bitangentData.push_back(bitangent.y);
        m_bitangentData.push_back(bitangent.z);
        m_bitangentData.push_back(bitangent.x);
        m_bitangentData.push_back(bitangent.y);
        m_bitangentData.push_back(bitangent.z);
    }
}

void MeshBuilder::uploadMesh() {
    if (buffer.vertexCount) {
        if (m_genTangents) {
            genTangents();
        }

        glBindBuffer(GL_ARRAY_BUFFER, buffer.bufferIDs[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * buffer.vertexCount, &m_vertexData[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.bufferIDs[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 2 * buffer.vertexCount, &m_texCoordData[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.bufferIDs[2]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * buffer.vertexCount, &m_normalData[0], GL_STATIC_DRAW);

        if (m_genTangents) {
            // Upload tangent and bi-tangent data
            glBindBuffer(GL_ARRAY_BUFFER, buffer.bufferIDs[3]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * buffer.vertexCount, &m_tangentData[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, buffer.bufferIDs[4]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * buffer.vertexCount, &m_bitangentData[0], GL_STATIC_DRAW);
            buffer.flags |= 1;
        }
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
    ++buffer.vertexCount;
}

//

Model::Model(VertexBuffer buffer, Material *mat): m_material{mat}, m_buffer{buffer} {}

Model::~Model() {
    delete m_material;
}

void Model::render(Shader *shader) {
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffer.bufferIDs[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer.bufferIDs[1]);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_buffer.bufferIDs[2]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);

    if (m_buffer.flags & 1) {
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);

        glBindBuffer(GL_ARRAY_BUFFER, m_buffer.bufferIDs[3]);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, m_buffer.bufferIDs[4]);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (m_material) {
        m_material->apply(shader);
    }

    glDrawArrays(GL_TRIANGLES, 0, m_buffer.vertexCount);

    if (m_buffer.flags & 1) {
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(3);
    }

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
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
    for (const auto &i: idx) {
        if (i.x < 0 || i.x >= vs.size() || i.z < 0 || i.z >= ns.size()) {
            return false;
        }
    }

    auto it = idx.begin();
    auto i0 = *it++;
    auto i1 = *it++;
    auto i2 = *it++;
    auto i3 = *it;

#define f_add(i, u, v) \
    mesh.normal(ns[i.z].x, ns[i.z].y, ns[i.z].z); \
    if (i.y < 0) { \
        mesh.texCoord(u, v); \
    } else { \
        mesh.texCoord(ts[i.y].x, ts[i.y].y); \
    } \
    mesh.vertex(vs[i.x].x, vs[i.x].y, vs[i.x].z)

    f_add(i0, 0, 0);
    f_add(i1, 0, 1);
    f_add(i2, 1, 1);
    f_add(i2, 1, 1);
    f_add(i3, 1, 0);
    f_add(i0, 0, 0);

#undef f_add

    return true;
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

    return new Model(mesh.buffer, mat);
}
