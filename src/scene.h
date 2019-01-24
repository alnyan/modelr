#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "render/common.h"
#include "gldynamicarray.h"
#include "modelobject.h"
#include "render/model.h"

struct DynamicBufferGroup {
    GlDynamicArray<DrawArraysIndirectCommand, GL_DRAW_INDIRECT_BUFFER, 0xFF> indirectCommands;
    GlDynamicArray<glm::mat4, GL_SHADER_STORAGE_BUFFER, S_SSBO_MODEL> modelMatrices;
    GlDynamicArray<MeshAttrib, GL_SHADER_STORAGE_BUFFER, S_SSBO_MESH_ATTRIB> meshAttribs;

    void generate() {
        indirectCommands.generate();
        modelMatrices.generate();
        meshAttribs.generate();
    }

    GLuint getSize() const {
        return indirectCommands.size;
    }

    void bind() {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommands.bufferID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, S_SSBO_MODEL, modelMatrices.bufferID);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, S_SSBO_MESH_ATTRIB, meshAttribs.bufferID);
    }
};

class Scene {
public:
    int addPartToBuffers(DynamicBufferGroup *group, const Part *p);
    void addDynamicObject(ModelObject *obj, glm::vec3 pos = glm::vec3(0), glm::quat rot = glm::quat());
    void addStaticObject(ModelObject *obj, glm::vec3 pos = glm::vec3(0), glm::quat rot = glm::quat());
    void init();

    DynamicBufferGroup staticModelObjectBuffers;
    DynamicBufferGroup dynamicModelObjectBuffers;

    std::list<ModelObject *> dynamicModelObjects;
    std::list<ModelObject *> staticModelObjects;
    std::list<ModelObject *> volatileModelObjects;
    std::list<GameObject *> genericObjects;
};
