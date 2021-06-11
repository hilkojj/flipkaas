precision mediump samplerCube;
precision mediump float;

layout (location = 0) out vec4 colorOut;
#if BLOOM
layout (location = 1) out vec3 brightColor;
#endif

in vec3 localPos;

uniform samplerCube skyBox;

void main()
{
    vec3 envColor = texture(skyBox, localPos).rgb;

    envColor = pow(envColor, vec3(1.0 / GAMMA));

    colorOut = vec4(envColor, 1.0);

    #if BLOOM
//    // check whether fragment output is higher than threshold, if so output as brightness color
//    float brightness = dot(colorOut.rgb, vec3(0.2126, 0.7152, 0.0722));
//    if (brightness > BLOOM_THRESHOLD)
//        brightColor.rgb = colorOut.rgb;
//    else
    brightColor.rgb = vec3(0);
    #endif
}