#include "material.h"
#include <iostream>
#include <fstream>
#include <string.h>

static Material *s_boundMaterial = nullptr;

Material::~Material() {
    delete m_map_Kd;
    delete m_map_Bump;
}

void Material::unbindAll() {
    s_boundMaterial = nullptr;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Material::apply(Shader *shader) {
    if (s_boundMaterial == this) {
        return;
    }
    s_boundMaterial = this;

    glActiveTexture(GL_TEXTURE0);
    if (m_map_Kd) {
        m_map_Kd->bind();
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glActiveTexture(GL_TEXTURE0 + 1);
    if (m_map_Bump) {
        m_map_Bump->bind();
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    auto l = shader->getUniformLocation("m_map_Kd");
    glUniform1i(l, 0);
    l = shader->getUniformLocation("m_map_Bump");
    glUniform1i(l, 1);

    shader->applyMaterial(this);
}

Material *Material::loadMtl(const std::string &path) {
    Material *res = new Material;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        const char *l = line.c_str();

        if (!strncmp(l, "Ns ", 3)) {
            // Shininess
            sscanf(l, "Ns %f", &res->uniformData.m_Ns);
        } else if (!strncmp(l, "Ka ", 3)) {
            // Ambient color
            sscanf(l, "Ka %f %f %f", &res->uniformData.m_Ka.x, &res->uniformData.m_Ka.y, &res->uniformData.m_Ka.z);
        } else if (!strncmp(l, "Kd ", 3)) {
            // Diffuse color
            sscanf(l, "Kd %f %f %f", &res->uniformData.m_Kd.x, &res->uniformData.m_Kd.y, &res->uniformData.m_Kd.z);
        } else if (!strncmp(l, "Ks ", 3)) {
            // Specular color
            sscanf(l, "Ks %f %f %f", &res->uniformData.m_Ks.x, &res->uniformData.m_Ks.y, &res->uniformData.m_Ks.z);
        } else if (!strncmp(l, "map_Kd ", 7)) {
            // Diffuse map
            char path[1024];
            sscanf(l, "map_Kd %s", path);

            res->m_map_Kd = Texture::loadPng(path);

            if (!res->m_map_Kd) {
                std::cerr << "Failed to load Diffuse Map" << std::endl << path << std::endl;
                delete res;
                return nullptr;
            }

            res->uniformData.m_Matopt |= 1;
        } else if (!strncmp(l, "map_Bump ", 8)) {
            // Normal map
            char path[1024];
            l += 8;
            float a, b, c;
            while (*l == ' ') {
                ++l;
            }

            if (*l == '-') {
                std::cout << l << std::endl;
                sscanf(l, "-s %f %f %f %s", &a, &b, &c, path);
            } else {
                strcpy(path, l);
            }

            res->m_map_Bump = Texture::loadPng(path);

            if (!res->m_map_Bump) {
                std::cerr << "Failed to load Normal Map" << std::endl << path << std::endl;
                delete res;
                return nullptr;
            }

            res->uniformData.m_Matopt |= 2;
        }
    }

    return res;
}
