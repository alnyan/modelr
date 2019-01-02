#include "material.h"

Material::~Material() {
    delete m_map_Kd;
}

void Material::apply(Shader *shader) {
    // TODO: set material parameters
}
