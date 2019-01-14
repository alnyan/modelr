#version 430 core
#extension GL_ARB_bindless_texture : require

struct Particle {
    vec4 pos;
    vec4 vel;
    ivec4 opts;
    float t0, t, u, v;
};

layout(std140,binding=S_UBO_TEXTURES) uniform mSamplerBuffer {
    sampler2D mTextures[S_TEXTURE_COUNT];
};

layout(std430, binding=S_SSBO_PARTICLE) buffer mParticleParams {
    Particle mParticles[];
};

uniform float mTime;

in vec2 mSourceTexCoord;
in flat uint mInstanceID;
out vec4 color;

void main() {
    float mLifespan = mParticles[mInstanceID].t;
    float mLifetime = mTime - mParticles[mInstanceID].t0;

    float trMod = (mLifespan - mLifetime) / mLifespan;
    vec3 rgb = texture(mTextures[3], mSourceTexCoord).rgb;
    float a = clamp(texture(mTextures[3], mSourceTexCoord).a * trMod, 0, 1);

    //color = vec4(1, 0, 0, 1);
    //color = vec4(trMod, trMod, trMod, 1);
    color = vec4(rgb, a);
}
