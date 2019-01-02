#include "material.h"
#include <iostream>
#include <fstream>
#include <string.h>

Material::~Material() {
    delete m_map_Kd;
    delete m_map_Bump;
}

void Material::apply(Shader *shader) {
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

    shader->setMaterial1i(MaterialShaderData::MAT_map_Kd, 0);
    shader->setMaterial1i(MaterialShaderData::MAT_map_Bump, 1);
    shader->setMaterial1i(MaterialShaderData::MAT_Matopt, 1);
    shader->setMaterial3f(MaterialShaderData::MAT_Kd, m_Kd);
    shader->setMaterial3f(MaterialShaderData::MAT_Ka, m_Ka);
    shader->setMaterial3f(MaterialShaderData::MAT_Ks, m_Ks);
    shader->setMaterial1f(MaterialShaderData::MAT_Ns, m_Ns);
}

Material *Material::loadMtl(const std::string &path) {
    Material *res = new Material;
    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line)) {
        const char *l = line.c_str();

        if (!strncmp(l, "Ns ", 3)) {
            // Shininess
            sscanf(l, "Ns %f", &res->m_Ns);
        } else if (!strncmp(l, "Ka ", 3)) {
            // Ambient color
            sscanf(l, "Ka %f %f %f", &res->m_Ka.x, &res->m_Ka.y, &res->m_Ka.z);
        } else if (!strncmp(l, "Kd ", 3)) {
            // Diffuse color
            sscanf(l, "Kd %f %f %f", &res->m_Kd.x, &res->m_Kd.y, &res->m_Kd.z);
        } else if (!strncmp(l, "Ks ", 3)) {
            // Specular color
            sscanf(l, "Ks %f %f %f", &res->m_Ks.x, &res->m_Ks.y, &res->m_Ks.z);
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
        } else if (!strncmp(l, "map_Bump ", 8)) {
            // Normal map
            char path[1024];
            l += 8;
            float a, b, c;
            while (*l == ' ') {
                ++l;
            }

            if (*l == '-') {
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
        }
    }

    return res;
}
