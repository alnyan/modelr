#version 430 core
#extension GL_ARB_bindless_texture : require

layout(std140,binding=S_UBO_TEXTURES) uniform mSamplerBuffer {
    sampler2D mTextures[S_TEXTURE_COUNT];
};

uniform float mLifetime;
uniform float mLifespan;

in vec2 mSourceTexCoord;
out vec4 color;

void main() {
    float trMod = (mLifespan - mLifetime) / mLifespan;
    vec3 rgb = texture(mTextures[3], mSourceTexCoord).rgb;
    float a = clamp(texture(mTextures[3], mSourceTexCoord).a * trMod, 0, 1);

    //color = vec4(trMod, trMod, trMod, 1);
    color = vec4(rgb, a);
}
