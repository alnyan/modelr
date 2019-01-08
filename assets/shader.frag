#version 430 core
#extension GL_ARB_bindless_texture : require

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;

uniform sampler2D mShadowMap0;
uniform sampler2D mShadowMap1;
uniform sampler2D mShadowMap2;
uniform sampler2D mShadowMap3;
uniform mat4 mDepth0;
uniform mat4 mDepth1;
uniform mat4 mDepth2;
uniform mat4 mDepth3;

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

const LightParams mLights[1] = {
    {
        vec3(10, 10, 10),
        vec3(0.5, 1, 1),
        100
    }
    //{
        //vec3(-3, 2, -3),
        //vec3(1, 0.7, 0.5),
        //10
    //}
};

// Hardcoded params (yet)
const float ambientIntensity = 0.2f;

const vec3 fogColor = vec3(0, 0.25, 0.25);
const float fogDensity = 0.05;

////

vec3 funDiffuse(vec3 inKd,
                vec3 lightDir,
                float lightDist,
                vec3 lightColor,
                float lightIntensity,
                vec3 normal) {
    float cosTheta = clamp(dot(normal, lightDir), 0, 1);

    return lightIntensity * lightColor * cosTheta * inKd / pow(lightDist, 2);
}

vec3 funSpecular(vec3 lightDir,
                 float lightDist,
                 vec3 lightColor,
                 float lightIntensity,
                 vec3 normal,
                 vec3 eyeDir) {
    vec3 rayReflect = normalize(reflect(-lightDir, normal));

    float cosAlpha = clamp(dot(rayReflect, eyeDir), 0, 1);

    return lightIntensity * lightColor * pow(cosAlpha, 4) / pow(lightDist, 2);
}

////

void main() {
    vec3 mShadowVertex = (mDepth0 * vec4(mSourceVertex, 1)).xyz;

    vec2 poissonDisk[4] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 )
    );
    mat4 bias = mat4(
        vec4(0.5, 0, 0, 0),
        vec4(0, 0.5, 0, 0),
        vec4(0, 0, 0.5, 0),
        vec4(0.5, 0.5, 0.5, 1)
    );
    float bias2 = 0.005;

    vec3 shadowCoord = (bias * vec4(mShadowVertex, 1)).xyz;

    float visibility = 1.0f;

    for (int i=0;i<4;i++){
        float shadowD = texture(mShadowMap1, shadowCoord.xy + poissonDisk[i] / 700.0).r;
        if (shadowD < shadowCoord.z - bias2) {
            visibility -= 0.2f;
        }
    }

    // Post-shadow

    MeshMaterial mat = mMaterials[mMeshMaterials[0]];
    vec3 baseKd = texture(mTextures[0], mSourceTexCoord).rgb;
    vec3 mapNormal = normalize(texture(mTextures[1], mSourceTexCoord).rgb * 2.0 - vec3(1.0));

    mat3 matTBN = transpose(mat3(
        normalize(mSourceTangent),
        normalize(mSourceBitangent),
        normalize(mSourceNormal)
    ));

    color = baseKd * ambientIntensity; // TODO: ambient
    vec3 eyeDir = normalize(matTBN * (-(mCameraDestination - mCameraPosition)).xyz);

    for (int i = 0; i < mLights.length(); ++i) {
        vec3 lightDir = matTBN * (mLights[i].mLightPos - mSourceVertex);
        color += visibility * funDiffuse(baseKd, normalize(lightDir), length(lightDir), mLights[i].mLightColor, mLights[i].mLightIntensity, mapNormal);
        color += visibility * funSpecular(normalize(lightDir), length(lightDir), mLights[i].mLightColor, mLights[i].mLightIntensity, mapNormal, eyeDir);
    }
}
