#version 430 core

struct Particle {
    vec4 pos;
    vec4 vel;
    ivec4 opts;
    float t0, t, u, v;
};

layout(location=S_PARTICLE_ATTRIB_VERTEX) in vec3 mVertex;
layout(location=S_PARTICLE_ATTRIB_TEXCOORD) in vec2 mTexCoord;

layout(std140, binding=S_UBO_SCENE) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std430, binding=S_SSBO_PARTICLE) buffer mParticleParams {
    Particle mParticles[];
};

uniform float mTime;

out vec2 mSourceTexCoord;
out flat uint mInstanceID;

void main() {
    mInstanceID = gl_InstanceID;
    Particle part = mParticles[gl_InstanceID];

    mSourceTexCoord = mTexCoord;

    float lifetime = part.t0;
    float lifespan = part.t;

    if (mTime - lifetime > lifespan) {
        return;
    }

    float scaleFactor = (mTime - lifetime) * 0.2 + 0.4;
    mat3 scale = mat3(
        vec3(scaleFactor, 0, 0),
        vec3(0, scaleFactor, 0),
        vec3(0, 0, scaleFactor)
    );

    vec3 camUp = vec3(mCameraMatrix[0].y, mCameraMatrix[1].y, mCameraMatrix[2].y);
    vec3 camRight = vec3(mCameraMatrix[0].x, mCameraMatrix[1].x, mCameraMatrix[2].x);

    vec4 vew = vec4(part.pos.xyz, 0) + vec4(scale * (camRight * mVertex.x + camUp * mVertex.y), 1);

    gl_Position = mProjectionMatrix * mCameraMatrix * vew;
}
