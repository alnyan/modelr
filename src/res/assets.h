#pragma once
#include <string>

#ifdef BUILD_CONFIG_RELEASE
#define ASSETS_BASE         "/usr/share/games/modelr"
#else
#define ASSETS_BASE         "assets"
#endif

namespace Assets {

    std::string getTexturePath(const std::string &t);
    std::string getMaterialPath(const std::string &t);
    std::string getModelPath(const std::string &t);
    std::string getShaderPath(const std::string &t);

};
