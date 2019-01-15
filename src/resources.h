#pragma once
#include <GL/glew.h>
#include <string>
#include "render/meshbuilder.h"
#include "render/model.h"
#include "render/common.h"

////

GLuint getTextureID(int index);
int getTextureIndex(const std::string &name);
int loadTextureFixed(GLuint id, const std::string &name, GLuint minOpt = GL_LINEAR_MIPMAP_LINEAR, GLuint magOpt = GL_LINEAR);
int loadTexture(const std::string &name, GLuint minOpt = GL_LINEAR_MIPMAP_LINEAR, GLuint magOpt = GL_LINEAR);

////

Model *getModelObject(int index);
Model *getModelObject(const std::string &name);
int createModelObject(const std::string &name, Model **mod);
int getModelIndex(const std::string &name);
int loadModel(MeshBuilder *mesh, const std::string &name);

////

MaterialUniformData *getMaterialObject(const std::string &name);
MaterialUniformData *getMaterialObject(int index);
int getMaterialIndex(const std::string &name);

int createMaterialObject(const std::string &name, MaterialUniformData **obj);
int createMaterial(const std::string &name);

int loadMaterials(const std::string &filename);
void uploadMaterials(GLuint buffer);
