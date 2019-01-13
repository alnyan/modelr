#version 430 core

layout(location=S_ATTRIB_VERTEX) in vec3 mVertex;
layout(location=S_ATTRIB_TEXCOORD) in vec2 mTexCoord;

layout(std140, binding=S_UBO_SCENE) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

uniform mat4 mModelMatrix;
uniform float mLifetime;

const vec3 squareVertices[] = {
    vec3(-0.5f, -0.5f, 0.0f),
    vec3( 0.5f, -0.5f, 0.0f),
    vec3(-0.5f,  0.5f, 0.0f),
    vec3( 0.5f,  0.5f, 0.0f)
};

out vec2 mSourceTexCoord;

void main() {
    mSourceTexCoord = mTexCoord;
    float scaleFactor = mLifetime * 0.5 + 0.1;
    mat3 scale = mat3(
        vec3(scaleFactor, 0, 0),
        vec3(0, scaleFactor, 0),
        vec3(0, 0, scaleFactor)
    );

    vec3 camUp = vec3(mCameraMatrix[0].y, mCameraMatrix[1].y, mCameraMatrix[2].y);
    vec3 camRight = vec3(mCameraMatrix[0].x, mCameraMatrix[1].x, mCameraMatrix[2].x);

    vec4 vew = mModelMatrix * vec4(scale * (camRight * mVertex.x + camUp * mVertex.y), 1);

    gl_Position = mProjectionMatrix * mCameraMatrix * vew;
}
