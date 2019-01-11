#version 430 core
#extension GL_ARB_shader_draw_parameters : enable

layout(location=S_ATTRIB_VERTEX) in vec3 mVertex;
layout(location=S_ATTRIB_TEXCOORD) in vec2 mTexCoord;
layout(location=S_ATTRIB_NORMAL) in vec3 mNormal;
layout(location=S_ATTRIB_TANGENT) in vec3 mTangent;
layout(location=S_ATTRIB_BITANGENT) in vec3 mBitangent;

layout(std140, binding=S_UBO_SCENE) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std430, binding=S_SSBO_MODEL) buffer mModelParams {
    mat4 mModelMatrices[];
};

out vec3 mSourceVertex;
out vec3 mSourceNormal;
out vec2 mSourceTexCoord;
out vec3 mSourceTangent;
out vec3 mSourceBitangent;

void main() {
    mat4 mModelMatrix = mModelMatrices[gl_DrawIDARB];

    mSourceVertex = (mModelMatrix * vec4(mVertex, 1.0)).xyz;
    mSourceNormal = mNormal;
    mSourceTexCoord = mTexCoord;
    mSourceTangent = mTangent;
    mSourceBitangent = mBitangent;

    gl_Position = mProjectionMatrix * mCameraMatrix * mModelMatrix * vec4(mVertex, 1.0);
}
