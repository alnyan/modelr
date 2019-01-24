#pragma once
#include "render/common.h"
#include "scene.h"

struct Camera {
    void setPosition(glm::vec3 pos);
    void setYaw(float yaw);
    void recalcMatrix();
    void translate(glm::vec3 delta);

    SceneUniformData uniformData;
    float yaw;
};

namespace Renderer {

    extern int width, height;
    extern Camera *camera;
    extern Scene *scene;

    int init_gl();
    int init();
    int loadData();
    int loadTextures();
    void render();

    bool shouldClose();
    void pollEvents();

    void setCursorCallback(GLFWcursorposfun cbfun);

}
