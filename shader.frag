#version 330 core

in vec3 mSourceVertex;
in vec3 mSourceNormal;
in vec2 mSourceTexCoord;
in vec3 mSourceTangent;
in vec3 mSourceBitangent;

uniform vec3 mCameraPosition;
uniform vec3 mCameraDestination;

uniform vec3 m_Ka;
uniform vec3 m_Kd;
uniform vec3 m_Ks;
uniform float m_Ns;
uniform sampler2D m_map_Kd;
uniform sampler2D m_map_Bump;
uniform int m_Matopt;

out vec3 color;

const vec3 lightColor = vec3(1, 1, 1);
const vec3 lightPos = vec3(3, 3, 3); // Assume this
const float ambientIntensity = 0.1f;
const float diffuseIntensity = 0.25f;

void main() {
    vec3 diffuseColor;
    if ((m_Matopt & 1) != 0) {
        diffuseColor = m_Kd * texture(m_map_Kd, mSourceTexCoord).rgb;
    } else {
        diffuseColor = m_Kd;
    }
    vec3 mapNormal = normalize((2 * texture(m_map_Bump, mSourceTexCoord).rgb) - vec3(1, 1, 1));
    mat3 matTBN = transpose(mat3(
        normalize(mSourceTangent),
        normalize(mSourceBitangent),
        normalize(mapNormal)
    ));

    vec3 lightVec = normalize(lightPos - mSourceVertex);
    float lightDist = length(lightVec);
    float cosTheta = clamp(dot(mSourceNormal, lightVec), 0, 1);
    float lightInt = cosTheta * diffuseIntensity / pow(lightDist, 2);

    vec3 eyeVec = normalize(mCameraPosition - mCameraDestination);
    vec3 lightReflect = reflect(-lightVec, mSourceNormal);

    float eyeDist = length(mCameraPosition - mSourceVertex);
    float cosAlpha = clamp(dot(eyeVec, lightReflect), 0, 1);
    float specularInt = diffuseIntensity * pow(cosAlpha, m_Ns) / pow(lightDist, 2);

    color =
        diffuseColor * lightInt +
        diffuseColor * m_Ka +
        lightColor * 10 * m_Ks * specularInt;
}
