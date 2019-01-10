#version 430 core
#extension GL_ARB_shader_draw_parameters : enable

#define S_ATTR_VERTEX       0
#define S_ATTR_TEX_COORD    1
#define S_ATTR_NORMAL       2

#define S_UBO_LIGHT0        2

layout(location = S_ATTR_VERTEX) in vec3 mVertex;
layout(location = S_ATTR_TEX_COORD) in vec2 mTexCoord;
layout(location = S_ATTR_NORMAL) in vec3 mNormal;

layout(std430,binding=1) buffer mModelParams {
    mat4 mModelMatrices[];
};

layout(std140,binding=2) uniform mLight0Buffer {
    mat4 mLightProjection[4];
    mat4 mLightMatrix;
    vec4 mLight0Pos;
};

uniform int mRenderCascade;

void main() {
    mat4 mModelMatrix = mModelMatrices[gl_DrawIDARB];

    gl_Position = mLightProjection[mRenderCascade] * mLightMatrix * mModelMatrix * vec4(mVertex, 1.0);
}
