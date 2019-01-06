#version 430 core
#extension GL_ARB_bindless_texture : require

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;

// UBOs: View-Projection matrix and camera params + textures
layout(std140,binding=0) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std140,binding=1) uniform mSamplerBuffer {
    sampler2D mTextures[256];
};
//

// Model parameter SSBOs
layout(std430,binding=1) buffer mModelParams {
    mat4 mModelMatrices[];
};

layout(std430,binding=2) buffer mMeshAttribs {
    int mMeshMaterials[];
};
//

layout(location = 0) out vec3 color;

// TODO: upload this from CPU, not hardcode
struct MeshMaterial {
    vec3 m_Kd;
    vec3 m_Ka;
    vec3 m_Ks;
    float m_Ns;
    int m_Matopt;
};

struct LightParams {
    vec3 mLightPos;
    vec3 mLightColor;
    float mLightIntensity;
};

const MeshMaterial mMaterials[1] = {
    {
        vec3(0, 1, 0),
        vec3(0.1, 0.1, 0.1),
        vec3(1, 1, 1),
        100,
        0
    }
};

const LightParams mLights[2] = {
    {
        vec3(3, 2, 3),
        vec3(1, 0, 0),
        10
    },
    {
        vec3(-3, 2, -3),
        vec3(0, 1, 0),
        10
    }
};

// Hardcoded params (yet)
const float ambientIntensity = 0.1f;


const vec3 fogColor = vec3(0, 0.25, 0.25);
const float fogDensity = 0.05;

////

vec3 funDiffuse(vec3 inKd,
                vec3 lightPos,
                vec3 lightColor,
                float lightIntensity,
                vec3 pos,
                vec3 normal) {
    vec3 lightDir = normalize(lightPos - pos);
    float lightDist = length(lightPos - pos);

    float cosTheta = clamp(dot(normal, lightDir), 0, 1);

    return lightIntensity * lightColor * cosTheta * inKd / pow(lightDist, 2);
}

vec3 funSpecular(vec3 lightPos,
                 vec3 lightColor,
                 float lightIntensity,
                 vec3 pos,
                 vec3 normal,
                 vec3 eyeDir) {
    vec3 lightDir = normalize(lightPos - pos);
    float lightDist = length(lightPos - pos);
    vec3 rayReflect = normalize(reflect(-lightDir, normal));

    float cosAlpha = clamp(dot(rayReflect, eyeDir), 0, 1);

    return lightIntensity * lightColor * pow(cosAlpha, 4) / pow(lightDist, 2);
}

////

void main() {
    MeshMaterial mat = mMaterials[mMeshMaterials[0]];
    vec3 baseKd;

    baseKd = texture(mTextures[0], mSourceTexCoord).rgb;

    color = vec3(0, 0, 0); // TODO: ambient

    vec3 eyeDir = normalize(vec3(mCameraPosition - mCameraDestination));

    for (int i = 0; i < mLights.length(); ++i) {
        color += funDiffuse(baseKd, mLights[i].mLightPos, mLights[i].mLightColor, mLights[i].mLightIntensity, mSourceVertex, mSourceNormal);
        color += funSpecular(mLights[i].mLightPos, mLights[i].mLightColor, mLights[i].mLightIntensity, mSourceVertex, mSourceNormal, eyeDir);
    }
}
