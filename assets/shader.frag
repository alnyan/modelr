#version 420 core

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;

// Manual padding to conform to std140 format (items are padded to 16 bytes)
struct MaterialSmallOpts {
    float m_Ns;
    int m_Matopt;
    int m_pad0, m_pad1;
};

layout(std140,binding=0) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std140,binding=1) uniform mMaterialParams {
    vec4 m_Ka;
    vec4 m_Kd;
    vec4 m_Ks;
    MaterialSmallOpts m_smallopts;
};

uniform mat4 mModelMatrix;

layout(location = 0) out vec3 color;

const vec3 lightColor = vec3(1, 1, 1);
const vec3 lightPos = vec3(5, 5, 5); // Assume this
const float ambientIntensity = 0.1f;
const float diffuseIntensity = 10;

const vec3 fogColor = vec3(0, 0.25, 0.25);
const float fogDensity = 0.05;

uniform sampler2D m_map_Kd;
uniform sampler2D m_map_Bump;

// Diffuse color function
vec3 funKd(vec3 kd, vec3 n, vec3 lightVec, float lightDist) {
    float cosTheta = clamp(dot(n, lightVec), 0, 1);
    return kd * cosTheta * diffuseIntensity / pow(lightDist, 2);
}

// Ambient color function
vec3 funKa(vec3 kd, vec3 ka) {
    return kd * ka;
}

// Specular color function
vec3 funKs(vec3 lightColor, vec3 normal, vec3 lightVec, vec3 eyeVec, float lightDist, float eyeDist) {
    vec3 lightReflect = reflect(-lightVec, normal);
    float cosAlpha = clamp(dot(eyeVec, lightReflect), 0, 1);
    float specInt = pow(cosAlpha, m_smallopts.m_Ns) / pow(lightDist, 2);
    return lightColor * specInt;
}

void main() {
    vec3 diffuseColor;
    if ((m_smallopts.m_Matopt & 1) != 0) {
        diffuseColor = m_Kd.rgb * texture(m_map_Kd, mSourceTexCoord).rgb;
    } else {
        diffuseColor = m_Kd.rgb;
    }

    // Get tangent-basis matrix
    float normalBumpiness = 1;
    mat3 matTBN;
    vec3 mapNormal;
    if ((m_smallopts.m_Matopt & 2) != 0) {
        mapNormal = (normalBumpiness * 2 * texture(m_map_Bump, mSourceTexCoord).rgb) - vec3(normalBumpiness);
        matTBN = transpose(mat3(
            normalize(mSourceTangent),
            normalize(mSourceBitangent),
            normalize(mapNormal)
        ));
    } else {
        mapNormal = mSourceNormal;
    }

    // Light params
    vec3 lightVec = normalize(lightPos - mSourceVertex);
    float lightDist = length(lightPos - mSourceVertex);
    vec3 eyeVec = normalize(mCameraPosition.xyz - mCameraDestination.xyz);
    float eyeDist = length(mCameraPosition.xyz - mSourceVertex);

    // Diffuse component
    vec3 res_Kd = funKd(diffuseColor, mapNormal, lightVec, lightDist);
    // Specular component
    vec3 res_Ks = funKs(lightColor, mapNormal, lightVec, eyeVec, lightDist, eyeDist);

    vec3 res_color = res_Kd + res_Ks + m_Kd.rgb * 0.1;

    float d = length(mSourceVertex - mCameraPosition.xyz);
    float fogFactor = 1 / exp(fogDensity * d);
    color = mix(fogColor, res_color, fogFactor);
}
