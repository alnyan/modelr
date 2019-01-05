#include "model.h"
#include <glm/glm.hpp>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <list>
#include "../res/assets.h"

//

using ObjVertexCoords = glm::vec<3, float>;
using ObjVertexIndex = std::list<glm::vec<3, int>>;
using ObjTexCoords = glm::vec<2, float>;

static_assert(sizeof(MeshBuilder::VertexFormat) == 8 * sizeof(GLfloat));
static_assert(sizeof(MeshBuilder::TangentFormat) == 6 * sizeof(GLfloat));

static Model *s_boundModel = nullptr;

//

MeshBuilder::MeshBuilder(bool hasTangents): m_genTangents{hasTangents} {
    glCreateBuffers(1, &buffer.vertexBufferID);
    if (hasTangents) {
        glCreateBuffers(1, &buffer.tangentBufferID);
    }
    glGenVertexArrays(1, &buffer.arrayID);
    m_u = m_v = 0;
    m_nx = m_ny = m_nz = 0;
    buffer.vertexCount = 0;
}

void MeshBuilder::genTangents() {
    for (int i = 0; i < buffer.vertexCount; i += 3) {
        VertexFormat a, b, c;
        a = m_vertexData[i];
        b = m_vertexData[i + 1];
        c = m_vertexData[i + 2];

        glm::vec3 dv0 = b.v - a.v;
        glm::vec3 dv1 = c.v - a.v;

        glm::vec2 dt0 = b.t - a.t;
        glm::vec2 dt1 = c.t - a.t;

        float r = 1.0f / (dt0.x * dt1.y - dt0.y * dt1.x);

        glm::vec3 tangent = -glm::normalize((dv0 * dt1.y - dv1 * dt0.y) * r);
        glm::vec3 bitangent = -glm::normalize((dv1 * dt0.x - dv0 * dt1.x) * r);

#define tround(x) \
        if (glm::abs(x) < 1e-6) { \
            x = 0; \
        }

        tround(tangent.x);
        tround(tangent.y);
        tround(tangent.z)
        tround(bitangent.x)
        tround(bitangent.y)
        tround(bitangent.z)

#undef tround

        for (int i = 0; i < 3; ++i) {
            m_tangentData.push_back({ tangent, bitangent });
        }
    }
}

void MeshBuilder::uploadMesh() {
    if (buffer.vertexCount) {
        if (m_genTangents) {
            genTangents();
        }

        assert(m_vertexData.size() == buffer.vertexCount);

        glBindVertexArray(buffer.arrayID);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(MeshBuilder::VertexFormat) * buffer.vertexCount, &m_vertexData[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshBuilder::VertexFormat), (GLvoid *) 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshBuilder::VertexFormat), (GLvoid *) (sizeof(glm::vec3)));
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshBuilder::VertexFormat), (GLvoid *) (sizeof(glm::vec3) + sizeof(glm::vec2)));

        if (m_genTangents) {
            assert(m_tangentData.size() == buffer.vertexCount);

            glBindBuffer(GL_ARRAY_BUFFER, buffer.tangentBufferID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TangentFormat) * buffer.vertexCount, &m_tangentData[0], GL_STATIC_DRAW);
            buffer.flags |= 1;
        }

        glBindVertexArray(0);
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
    m_vertexData.push_back({
            {x, y, z},
            {m_u, m_v},
            {m_nx, m_ny, m_nz}
    });
    m_u = m_v = 0;
    m_nx = m_ny = m_nz = 0;
    ++buffer.vertexCount;
}

//

Model::Model(VertexBuffer buffer, Material *mat): m_material{mat}, m_buffer{buffer} {}

Model::~Model() {
    delete m_material;
}

void Model::bind(Shader *shader) {
    if (s_boundModel == this) {
        return;
    }
    s_boundModel = this;

    glBindVertexArray(m_buffer.arrayID);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    if (m_buffer.flags & 1) {
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
    }

    if (m_material) {
        m_material->apply(shader);
    }
}

void Model::unbindAll() {
    s_boundModel = nullptr;
    glDisableVertexAttribArray(4);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Model::unbind(Shader *shader) {
    s_boundModel = nullptr;

    if (m_buffer.flags & 1) {
        glDisableVertexAttribArray(4);
        glDisableVertexAttribArray(3);
    }

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Model::render(Shader *shader) {
    glDrawArrays(GL_TRIANGLES, 0, m_buffer.vertexCount);
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
    std::ifstream file(Assets::getModelPath(path));
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
