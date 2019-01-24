#include "renderer.h"
#include "input.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "modelobject.h"
#include "resources.h"
//

//

int init(void) {
    ModelObject *obj0 = new ModelObject;
    obj0->setModel(getModelObject("model.obj"));

    Renderer::scene->addStaticObject(obj0);

    return 0;
}


////

void updatePlayer(double t, double dt) {
    int fwd = Input::get(GLFW_KEY_W) - Input::get(GLFW_KEY_S);
    int strafe = Input::get(GLFW_KEY_A) - Input::get(GLFW_KEY_D);
    int fly = Input::get(GLFW_KEY_Q) - Input::get(GLFW_KEY_Z);

    glm::vec3 walkDir(
        fwd * glm::cos(Renderer::camera->yaw) + strafe * glm::sin(Renderer::camera->yaw),
        fly,
        fwd * glm::sin(Renderer::camera->yaw) - strafe * glm::cos(Renderer::camera->yaw)
    );

    if (fwd != 0 || strafe != 0 || fly != 0) {
        auto walkDelta = 10 * (float) dt * glm::normalize(walkDir);

        Renderer::camera->translate(walkDelta);
    }

    Renderer::camera->recalcMatrix();
    //if (s_walk || s_strafe || s_fly) {
        //auto walkDir = glm::normalize(glm::vec4(
            //s_walk * cos(s_cameraYaw) + s_strafe * sin(s_cameraYaw),
            //s_fly,
            //s_walk * sin(s_cameraYaw) - s_strafe * cos(s_cameraYaw),
            //0
        //));
        //auto walkDelta = walkDir * (float) dt * 10.0f;
        //s_cameraVelocity = 0.01f * glm::vec3(walkDelta) + glm::vec3(s_cameraRotDelta, 0);

        //s_sceneUniformData.m_cameraPosition += walkDelta;
        //s_sceneUniformData.m_cameraDestination += walkDelta;
    //} else {
        //s_cameraVelocity = glm::vec3(s_cameraRotDelta, 0);
    //}
    //s_cameraRotDelta = glm::mix(glm::vec2(0), s_cameraRotDelta, 0.3);

    //s_sceneUniformData.m_cameraMatrix = glm::lookAt(
        //glm::vec3(s_sceneUniformData.m_cameraPosition),
        //glm::vec3(s_sceneUniformData.m_cameraDestination),
        //{ 0, 1, 0 }
    //);

    //if (m_width && m_height) {
        //float fov = 45.0f;
        //float aspect = ((float) m_height) / ((float) m_width);
        //s_sceneUniformData.m_projectionMatrix = glm::perspective(fov, 1 / aspect, 0.1f, 100.0f);
    //}
}

void updateLight0(double t) {
    //if (m_width != 0 && m_height != 0) {
        //s_light0UniformData.m_lightMatrix = glm::lookAt(
            //glm::vec3(s_light0UniformData.m_lightPosition),
            //glm::vec3(0, 0, 0),
            //glm::vec3(0, 1, 0)
        //);

        //auto lightP = s_light0UniformData.m_lightMatrix * glm::vec4(glm::vec3(s_sceneUniformData.m_cameraPosition), 1);
        //float cascades[S_SHADOW_CASCADES] {
            //20,
            //40,
            //80,
            //100
        //};

        //for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
            //s_light0UniformData.m_projectionMatrix[i] = glm::ortho(
                //lightP.x - cascades[i],
                //lightP.x + cascades[i],
                //lightP.y - cascades[i],
                //lightP.y + cascades[i],
                //-lightP.z - cascades[i],
                //-lightP.z + cascades[i]
            //);
        //}
    //}

}

//void addParticle(glm::vec3 pos, glm::vec3 vel, float t0, float t) {
    //int i = s_lastParticle++;
    //s_lastParticle %= R_PARTICLE_MAX;

    //s_particles[i] = {
        //glm::vec4(pos, 0), glm::vec4(vel, 0), glm::ivec4(0), t0, t
    //};
//}

void update(double t, double dt) {
    updatePlayer(t, dt);
    updateLight0(t);
    //if (s_phys) {
        //if (t - s_lastParticleTime > 0.05) {
            //s_lastParticleTime = t;
            //auto r0 = rand() / (float) RAND_MAX;
            //auto r1 = rand() / (float) RAND_MAX;
            //auto r2 = rand() / (float) RAND_MAX;
            //auto r3 = rand() / (float) RAND_MAX;

            //float y = 0.06;
            //float sx = 0.005;
            //float sy = 0.1;

            //addParticle({ 6 + r2 * sy * 2 - sy, 1, 6 + r3 * sy * 2 - sy }, { r0 * sx * 2 - sx, y, r1 * sx * 2 - sx }, t, 6);
        //}

        //for (int i = 0; i < R_PARTICLE_MAX; ++i) {
            //Particle &p = s_particles[i];

            //if ((t - p.t0) - p.t > 0 || p.t < 1 || p.t0 == 0) {
            //} else {
                //p.pos += p.vel;
                //p.vel *= 0.9998;
            //}
        //}
    //}
}

////

//
void cursorPosCallback(GLFWwindow *win, double x, double y) {
    if (Renderer::width == 0 || Renderer::height == 0) {
        return;
    }

    double cx = (x - Renderer::width / 2) / Renderer::width;
    double cy = (y - Renderer::height / 2) / Renderer::height;

    glfwSetCursorPos(win, Renderer::width / 2, Renderer::height / 2);

    //s_cameraPitch -= cy;
    //s_cameraYaw += cx;
    Renderer::camera->yaw += cx;
    Renderer::camera->uniformData.m_cameraDestination = Renderer::camera->uniformData.m_cameraPosition +
        glm::vec4(glm::cos(Renderer::camera->yaw), 0, glm::sin(Renderer::camera->yaw), 0);
    //s_sceneUniformData.m_cameraDestination = s_sceneUniformData.m_cameraPosition + glm::vec4(cos(s_cameraYaw), 0, glm::sin(s_cameraYaw), 0);
    //s_cameraRotDelta = glm::mix(s_cameraRotDelta, glm::vec2(s_cameraYaw, 0) + s_cameraRotDelta, 0.2);
}

//void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    //if (key == GLFW_KEY_T && action == 1) {
        //++s_viewType;
        //s_viewType %= 6;
    //}
    //if (key == GLFW_KEY_EQUAL && action == 1) {
        //++s_shadowControl;
    //}
    //if (key == GLFW_KEY_MINUS && action == 1) {
        //--s_shadowControl;
    //}
    //if (key == GLFW_KEY_R && action == 1) {
        //++s_renderType;
        //s_renderType %= 2;
    //}
    //if (key == GLFW_KEY_W) {
        //s_walk = !!action;
    //}
    //if (key == GLFW_KEY_S) {
        //s_walk = -!!action;
    //}
    //if (key == GLFW_KEY_A) {
        //s_strafe = !!action;
    //}
    //if (key == GLFW_KEY_D) {
        //s_strafe = -!!action;
    //}
    //if (key == GLFW_KEY_SPACE) {
        //s_fly = !!action;
    //}
    //if (key == GLFW_KEY_LEFT_SHIFT) {
        //s_fly = -!!action;
    //}

    //if (key == GLFW_KEY_P && action == 1) {
        //s_phys = !s_phys;
    //}
//}

//

int main() {
    if (Renderer::init() != 0) {
        return -1;
    }

    Renderer::setCursorCallback(cursorPosCallback);

    Input::listen(GLFW_KEY_W);
    Input::listen(GLFW_KEY_S);
    Input::listen(GLFW_KEY_A);
    Input::listen(GLFW_KEY_D);
    Input::listen(GLFW_KEY_Q);
    Input::listen(GLFW_KEY_Z);

    if (init() != 0) {
        return -1;
    }

    double t0 = glfwGetTime();
    while (!Renderer::shouldClose()) {
        double t = glfwGetTime();
        auto dt = t - t0;
        t0 = t;

        update(t, dt);

        Renderer::render();
        Renderer::pollEvents();
    }

    return 0;
}
