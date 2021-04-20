precision mediump float;

struct PointLight
{
    vec3 position;

//    float constant;
//    float linear;
//    float quadratic;
    vec3 attenuation; // x: constant, y: linear, z: quadratic

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct DirectionalLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


in vec3 v_position;
in vec2 v_textureCoord;
in mat3 v_TBN;

out vec3 colorOut;

uniform vec3 diffuse;
uniform vec4 specular;  // 4th component is Exponent

uniform int useDiffuseTexture;
uniform sampler2D diffuseTexture;

uniform int useSpecularMap;
uniform sampler2D specularMap;

uniform int useNormalMap;
uniform sampler2D normalMap;

uniform vec3 camPosition;

#ifndef NR_OF_DIR_LIGHTS
#define NR_OF_DIR_LIGHTS 0
#endif

#ifndef NR_OF_POINT_LIGHTS
#define NR_OF_POINT_LIGHTS 0
#endif

#if NR_OF_DIR_LIGHTS
uniform DirectionalLight dirLights[NR_OF_DIR_LIGHTS];    // TODO: uniform buffer object?
#endif

#if NR_OF_POINT_LIGHTS
uniform PointLight pointLights[NR_OF_POINT_LIGHTS];    // TODO: uniform buffer object?
#endif


void calcPointLight(PointLight light, vec3 normal, vec3 viewDir, inout vec3 totalDiffuse, inout vec3 totalSpecular, inout vec3 totalAmbient)
{
    vec3 lightDir = normalize(light.position - v_position);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specular.a * 8.);

    // attenuation
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;

    float distance    = length(light.position - v_position);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // combine results
    totalAmbient  += light.ambient  * attenuation;
    totalDiffuse += light.diffuse  * diff * attenuation;
    totalSpecular  += light.specular * spec * attenuation;
}

void calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, inout vec3 totalDiffuse, inout vec3 totalSpecular, inout vec3 totalAmbient)
{
    // diffuse shading
    float diff = max(dot(normal, -light.direction), 0.0);

    // specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(-light.direction + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specular.a * 8.);

    // combine results
    totalAmbient += light.ambient;
    totalDiffuse += light.diffuse * diff;
    totalSpecular += light.specular * spec;
}

// PHONG
void main()
{
    vec3 normal = vec3(0, 0, 1);    // normal will be in World space.

    // normal map:
    if (useNormalMap == 1)
    {
        normal = texture(normalMap, v_textureCoord).xyz;
        normal = normal * 2. - 1.;
    }
    normal = normalize(v_TBN * normal);

    vec3 diffuseColor = diffuse;
    if (useDiffuseTexture == 1)
    {
        diffuseColor = texture(diffuseTexture, v_textureCoord).rgb;
        diffuseColor = pow(diffuseColor, vec3(GAMMA)); // sRGB to linear space. https://learnopengl.com/Advanced-Lighting/Gamma-Correction
    }

    colorOut = diffuseColor;

    vec3 totalDiffuseLight = vec3(0);
    vec3 totalSpecularLight = vec3(0);
    vec3 totalAmbientLight = vec3(0);

    vec3 viewDir = normalize(camPosition - v_position);

    #if NR_OF_POINT_LIGHTS
    {   // Light points

        for (int i = 0; i < NR_OF_POINT_LIGHTS; i++)
            calcPointLight(pointLights[i], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
    }
    #endif

    #if NR_OF_DIR_LIGHTS
    {   // Directional lights

        for (int i = 0; i < NR_OF_DIR_LIGHTS; i++)
            calcDirLight(dirLights[i], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
    }
    #endif

    // diffuse & ambient:
    colorOut *= vec3(max(totalAmbientLight.r, totalDiffuseLight.r), max(totalAmbientLight.g, totalDiffuseLight.g), max(totalAmbientLight.b, totalDiffuseLight.b));

    // specularity:
    vec3 specularColor = specular.rgb;
    if (useSpecularMap == 1)
        specularColor = texture(specularMap, v_textureCoord).rgb;

    colorOut += specularColor * totalSpecularLight;

    // gamma correction:
    colorOut = pow(colorOut, vec3(1.0 / GAMMA));
}
