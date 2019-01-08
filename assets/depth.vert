#version 430 core
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 mVertex;
layout(location = 1) in vec2 mTexCoord;
layout(location = 2) in vec3 mNormal;

layout(std140,binding=0) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std430,binding=1) buffer mModelParams {
    mat4 mModelMatrices[];
};

void main() {
    mat4 mModelMatrix = mModelMatrices[gl_DrawIDARB];

    gl_Position = mProjectionMatrix * mCameraMatrix * mModelMatrix * vec4(mVertex, 1.0);
}
