#pragma once
#include "meshbuilder.h"
#include "model.h"

/**
 * @brief Facilities for loading Wavefront .obj/.mtl files
 */
namespace Wavefront {

/**
 * @brief Loads model mesh data from .obj file
 */
bool loadObj(Model *m, GLuint &material, MeshBuilder *mesh, const char *file);

};
