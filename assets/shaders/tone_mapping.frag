precision mediump float;

in vec2 v_texCoords;
out vec3 colorOut;

uniform sampler2D hdrImage;
uniform sampler2D blurImage;
uniform float exposure;

void main()
{
    vec3 hdr = texture(hdrImage, v_texCoords).rgb;
    hdr += texture(blurImage, v_texCoords).rgb;
    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);

    colorOut = mapped;
}