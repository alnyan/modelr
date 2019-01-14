#version 430 core
#extension GL_ARB_bindless_texture : require

struct Material {
    vec4 m_Kd;
    vec4 m_Ka;
    vec4 m_Ks;
    ivec4 m_maps;
};

struct MeshAttrib {
    int mMaterialIndex;
    int pad[3];
    vec4 mVelocity;
    //int mMaterialIndex;
    //float mVelX, mVelY, mVelZ;
};

struct LightParams {
    vec4 mLightPos;
    vec3 mLightColor;
    float mLightIntensity;
};

////

const float cascades[S_SHADOW_CASCADES] = {
    15,
    35,
    75,
    95
};

////

uniform vec3 mCameraVelocity;

////

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;
in flat uint mDrawID;

// UBOs: View-Projection matrix and camera params + textures
layout(std140,binding=S_UBO_SCENE) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std140,binding=S_UBO_TEXTURES) uniform mSamplerBuffer {
    sampler2D mTextures[S_TEXTURE_COUNT];
};

layout(std140,binding=S_UBO_LIGHT0) uniform mLight0Buffer {
    mat4 mLightProjection[S_SHADOW_CASCADES];
    mat4 mLightMatrix;
    vec4 mLight0Pos;
};

layout(std140,binding=S_UBO_MATERIALS) uniform mMaterialBuffer {
    Material mMaterials[S_MATERIAL_COUNT];
};

//

// Model parameter SSBOs
layout(std430, binding=S_SSBO_MODEL) buffer mModelParams {
    mat4 mModelMatrices[];
};

layout(std430, binding=S_SSBO_MESH_ATTRIB) buffer mMeshAttribBuffer {
    MeshAttrib mMeshAttribs[];
};
//

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 velColor;

LightParams mLights[1] = {
    // Sunlight
    {
        vec4(-mLight0Pos.xyz, 0),
        vec3(1, 1, 0.6),
        1
    }
};

// Hardcoded params (yet)
const float ambientIntensity = 0.2;

const vec3 fogColor = vec3(0, 0.25, 0.25);
const float fogDensity = 0.05;

////

vec3 funDiffusePoint(vec3 inKd,
                vec3 lightDir,
                float lightDist,
                vec3 lightColor,
                float lightIntensity,
                vec3 normal) {
    float cosTheta = clamp(dot(normal, lightDir), 0, 1);

    return lightIntensity * lightColor * cosTheta * inKd / pow(lightDist, 2);
}

vec3 funSpecularPoint(vec3 lightDir,
                 float lightDist,
                 vec3 lightColor,
                 float lightIntensity,
                 float specularExp,
                 vec3 normal,
                 vec3 eyeDir) {
    vec3 rayReflect = normalize(reflect(-lightDir, normal));

    float cosAlpha = clamp(dot(rayReflect, eyeDir), 0, 1);

    return lightIntensity * lightColor * pow(cosAlpha, specularExp) / pow(lightDist, 2);
}

////

vec3 funDiffuseDir(vec3 inKd,
                   vec3 lightDir,
                   vec3 lightColor,
                   float lightIntensity,
                   vec3 normal) {
    float cosTheta = clamp(dot(normal, lightDir), 0, 1);

    return lightIntensity * lightColor * cosTheta * inKd;
}

vec3 funSpecularDir(vec3 lightDir,
                   vec3 lightColor,
                   float lightIntensity,
                   float specularExp,
                   vec3 normal,
                   vec3 eyeDir) {
    vec3 rayReflect = normalize(reflect(-lightDir, normal));

    float cosAlpha = clamp(dot(rayReflect, eyeDir), 0, 1);

    return lightIntensity * lightColor * pow(cosAlpha, specularExp);
}

////

const float shadowHardness = 0.2;
const vec2 poissonDisk[4] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 )
);
const mat4 shadowBias = mat4(
    vec4(0.5, 0, 0, 0),
    vec4(0, 0.5, 0, 0),
    vec4(0, 0, 0.5, 0),
    vec4(0.5, 0.5, 0.5, 1)
);
const float shadowBias2 = 0.005;

float shadowFactor(int idx) {
    vec3 shadowVertex = (mLightProjection[idx] * mLightMatrix * vec4(mSourceVertex, 1)).xyz;
    vec3 shadowCoord = (shadowBias * vec4(shadowVertex, 1)).xyz;
    float visibility = 1.0f;
    for (int i = 0; i < 4; ++i){
        float shadowD;
        shadowD = texture(mTextures[S_SHADOW_MAP_0 + idx], shadowCoord.xy + poissonDisk[i] / 700.0).r;

        if (shadowD < shadowCoord.z - shadowBias2) {
            visibility -= shadowHardness;
        }
    }

    return visibility;
}

void main() {
    float visibility = 1.0f;

    vec3 vertexD = (mLightMatrix * mCameraPosition).xyz - (mLightMatrix * vec4(mSourceVertex, 1)).xyz;

    for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
        if (abs(vertexD.x) < cascades[i] &&
            abs(vertexD.y) < cascades[i] &&
            abs(vertexD.z) < cascades[i]) {
            visibility = shadowFactor(i);
            break;
        }
    }

    // Post-shadow
    int matIndex = mMeshAttribs[mDrawID].mMaterialIndex;
    vec3 vel = mMeshAttribs[mDrawID].mVelocity.xyz - mCameraVelocity;
    int m_map_Kd = S_TEXTURE_UNDEFINED;
    int m_map_Bump = -1;
    float m_Ns = 100;

    if (matIndex >= 0) {
        m_map_Kd = mMaterials[matIndex].m_maps.x;
        if (m_map_Kd < 0) {
            m_map_Kd = S_TEXTURE_UNDEFINED;
        }
        m_map_Bump = mMaterials[matIndex].m_maps.y;

        m_Ns = mMaterials[matIndex].m_Ks.w;
    }

    vec3 baseKd;
    vec3 mapNormal;
    mat3 matTBN;

    baseKd = texture(mTextures[m_map_Kd], mSourceTexCoord).rgb;

    if (m_map_Bump >= 0) {
        mapNormal = normalize(texture(mTextures[m_map_Bump], mSourceTexCoord).rgb * 2.0 - vec3(1.0));

        matTBN = transpose(mat3(
            normalize(mSourceTangent),
            normalize(mSourceBitangent),
            normalize(mSourceNormal)
        ));
    } else {
        mapNormal = mSourceNormal;
        matTBN = mat3(1);
    }

    vec4 projVel = mProjectionMatrix * mCameraMatrix * vec4(vel, 0);

    velColor = vec3(projVel.xy, 0);

    color = baseKd * ambientIntensity;
    vec3 eyeDir = normalize(matTBN * (-(mCameraDestination - mCameraPosition)).xyz);

    for (int i = 0; i < mLights.length(); ++i) {
        if (mLights[i].mLightPos.w > 0.5) {
            vec3 lightDir = matTBN * (mLights[i].mLightPos.xyz - mSourceVertex);
            color += visibility * funDiffusePoint(baseKd, normalize(lightDir), length(lightDir), mLights[i].mLightColor, mLights[i].mLightIntensity, mapNormal);
            color += visibility * funSpecularPoint(normalize(lightDir), length(lightDir), mLights[i].mLightColor, mLights[i].mLightIntensity, m_Ns, mapNormal, eyeDir);
        } else {
            vec3 lightDir = matTBN * normalize(-mLights[i].mLightPos.xyz);
            color += visibility * funDiffuseDir(baseKd, lightDir, mLights[i].mLightColor, mLights[i].mLightIntensity, mapNormal);
            color += visibility * funSpecularDir(lightDir, mLights[i].mLightColor, mLights[i].mLightIntensity, m_Ns, mapNormal, eyeDir);
        }
    }
}
