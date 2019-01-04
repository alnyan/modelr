#include "assets.h"

std::string Assets::getTexturePath(const std::string &text) {
    return std::string(ASSETS_BASE "/") + text;
}

std::string Assets::getMaterialPath(const std::string &text) {
    return std::string(ASSETS_BASE "/") + text;
}

std::string Assets::getModelPath(const std::string &text) {
    return std::string(ASSETS_BASE "/") + text;
}

std::string Assets::getShaderPath(const std::string &text) {
    return std::string(ASSETS_BASE "/") + text;
}
