precision mediump float;
precision mediump samplerCube;

out vec4 colorOut;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float sampleDelta;

#define PI 3.14159265359

void main()
{
    // the sample direction equals the hemisphere's orientation
    vec3 normal = normalize(localPos);

    vec3 irradiance = vec3(0.);

    vec3 up    = vec3(0., 1., 0.);
    vec3 right = normalize(cross(up, normal));
    up         = normalize(cross(normal, right));

    float nrSamples = 0.;
    for(float phi = 0.; phi < 2. * PI; phi += sampleDelta)
    {
        for(float theta = 0.; theta < .5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            vec3 sampl = texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);

            // fix for issue where some pixels are increadibly bright, causing weird patterns. https://learnopengl.com/PBR/IBL/Diffuse-irradiance#comment-4935613773
            sampl = min(vec3(100), sampl);

            irradiance += sampl;
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1. / nrSamples);

    colorOut = vec4(irradiance, 1.);
}
