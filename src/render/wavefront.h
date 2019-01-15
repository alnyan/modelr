#pragma once
#include "meshbuilder.h"
#include "model.h"
#include <string>
#include <list>

/**
 * @brief Facilities for loading Wavefront .obj/.mtl files
 */
namespace Wavefront {

/**
 * @brief Loads model mesh data from .obj file
 */
bool loadObj(Model *m, std::list<std::string> *reqMaterials, MeshBuilder *mesh, const std::string &file);

};
