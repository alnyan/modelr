#version 330 core

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;

uniform vec3 mCameraPosition;
uniform vec3 mCameraDestination;
uniform mat4 mModelMatrix;

uniform vec3 m_Ka;
uniform vec3 m_Kd;
uniform vec3 m_Ks;
uniform float m_Ns;
uniform sampler2D m_map_Kd;
uniform sampler2D m_map_Bump;
uniform int m_Matopt;

out vec3 color;

const vec3 lightColor = vec3(1, 1, 1);
const vec3 lightPos = vec3(5, 5, 5); // Assume this
const float ambientIntensity = 0.1f;
const float diffuseIntensity = 10;

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
    float specInt = pow(cosAlpha, m_Ns) / pow(lightDist, 2);
    return lightColor * specInt;
}

void main() {
    vec3 diffuseColor;
    if ((m_Matopt & 1) != 0) {
        diffuseColor = m_Kd * texture(m_map_Kd, mSourceTexCoord).rgb;
    } else {
        diffuseColor = m_Kd;
    }

    // Get tangent-basis matrix
    float normalBumpiness = 10;
    mat3 matTBN;
    vec3 mapNormal;
    if ((m_Matopt & 2) != 0) {
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
    vec3 eyeVec = normalize(mCameraPosition - mCameraDestination);
    float eyeDist = length(mCameraPosition - mSourceVertex);

    // Diffuse component
    vec3 res_Kd = funKd(diffuseColor, mapNormal, lightVec, lightDist);
    // Specular component
    vec3 res_Ks = funKs(lightColor, mapNormal, lightVec, eyeVec, lightDist, eyeDist);

    color =
        res_Kd +
        res_Ks;
        //0;
}
