#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <list>

#define GAME_OBJECT_CONSTRUCTOR_PARAMS() GameObject *parent = nullptr, glm::vec3 pos = glm::vec3(0), glm::quat rot = glm::quat()
#define GAME_OBJECT(className) \
    public: \
        className(GAME_OBJECT_CONSTRUCTOR_PARAMS())

#define GAME_PROPERTY_PRIM(name, vtype, access) \
    public: \
        template<bool V = access> typename std::enable_if<V>::type set##name(vtype v); \
    public: \
        vtype get##name() const; \
    private: \
        vtype m_##name

#define GAME_PROPERTY(name, vtype, access) \
    public: \
        template<bool V = access> typename std::enable_if<V>::type set##name(const vtype &v); \
    public: \
        const vtype &get##name() const; \
    private: \
        vtype m_##name

#define GAME_CONSTRUCTOR(className) \
    className::className(GameObject *parent, glm::vec3 pos, glm::quat rot) : GameObject(parent, pos, rot)

class GameObject {
public:
    ////

    /**
     * @brief Tells engine how dynamic the object is:
     */
    enum class OptimizationClass {
        /**
         * @brief The object is static and does not move at all
         */
        OPTIMIZE_STATIC,
        /**
         * @brief The object is dynamic, but does not enter/leave scene frequently
         */
        OPTIMIZE_DYNAMIC,
        /**
         * @brief The object is dynamic and is frequently added/removed from scene
         */
        OPTIMIZE_DYNAMIC_VOLATILE
    };

    ////

    GameObject(GameObject *parent = nullptr, glm::vec3 pos = glm::vec3(0), glm::quat rot = glm::quat());

    virtual void cloneFrom(GameObject *other);

    // Params
    void translate(glm::vec3 pos);
    void rotate(glm::vec3 rot);

    // Physics
    void simulate(double dt);

    // State
    virtual void update(double t, double dt);

private:
    // General properties
    GAME_PROPERTY_PRIM(Parent, GameObject *, false) = nullptr;
    GAME_PROPERTY_PRIM(PhysicsEnabled, bool, true) = false;
    GAME_PROPERTY_PRIM(OptimizationClass, OptimizationClass, true) = OptimizationClass::OPTIMIZE_STATIC;
    GAME_PROPERTY_PRIM(Cloneable, bool, true) = true;
    GAME_PROPERTY_PRIM(IsClone, bool, true) = false;

    // Physic properties
    GAME_PROPERTY(Velocity, glm::vec3, true) = glm::vec3(0);
    GAME_PROPERTY(Torque, glm::quat, true);
    GAME_PROPERTY(LocalPosition, glm::vec3, true) = glm::vec3(0);
    GAME_PROPERTY(LocalRotation, glm::quat, true);
};

template<typename T> T *cloneGameObject(T *other) {
    T *res = new T;
    res->cloneFrom(other);
    return res;
}
