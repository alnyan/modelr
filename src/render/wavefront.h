#pragma once
#include "meshbuilder.h"

/**
 * @brief Facilities for loading Wavefront .obj/.mtl files
 */
namespace Wavefront {

/**
 * @brief Loads model mesh data from .obj file
 */
bool loadObj(GLuint &model, GLuint &material, MeshBuilder *mesh, const char *file);

};
