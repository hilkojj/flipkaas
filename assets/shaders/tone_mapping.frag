precision mediump float;

in vec2 v_texCoords;
out vec3 colorOut;

uniform sampler2D hdrImage;
#if BLOOM
uniform sampler2D blurImage;
#endif
uniform float exposure;

void main()
{
    vec3 hdr = texture(hdrImage, v_texCoords).rgb;
    #if BLOOM
    hdr += texture(blurImage, v_texCoords).rgb;
    #endif
    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);

    colorOut = mapped;
}