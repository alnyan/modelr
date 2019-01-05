#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "../gameobject.h"

//class Model;

class MeshObject: public GameObject {
public:
    MeshObject(GLint model, GameObject *parent = nullptr);

    bool compare(MeshObject *o) const;

    void onUpdatePosition() override;

    //void render();
    //void setShader(Shader *s);
    void setRenderMode(GLuint mode);

private:
    GLuint m_renderMode = GL_TRIANGLES;
    //Shader *m_shader;
    GLint m_model;
    //Model *m_model;
    glm::mat4 m_modelMatrix;
};
