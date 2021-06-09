precision mediump samplerCube;
precision mediump float;

out vec4 color;

in vec3 localPos;

uniform samplerCube skyBox;

void main()
{
    vec3 envColor = texture(skyBox, localPos).rgb;

    envColor = pow(envColor, vec3(1.0 / GAMMA));

    color = vec4(envColor, 1.0);
}