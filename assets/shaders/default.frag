precision mediump float;
precision mediump sampler2DShadow;

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

#if SHADOWS
struct DirectionalShadowLight
{
    mat4 shadowSpace;
    DirectionalLight light;
};
#endif

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

uniform int useShadows; // todo: different shader for models that dont receive shadows? Sampling shadowmaps is expensive

uniform vec3 camPosition;

#ifndef NR_OF_DIR_LIGHTS
#define NR_OF_DIR_LIGHTS 0
#endif

#ifndef NR_OF_DIR_SHADOW_LIGHTS
#define NR_OF_DIR_SHADOW_LIGHTS 0
#endif

#ifndef NR_OF_POINT_LIGHTS
#define NR_OF_POINT_LIGHTS 0
#endif

#if NR_OF_DIR_LIGHTS
uniform DirectionalLight dirLights[NR_OF_DIR_LIGHTS];    // TODO: uniform buffer object?
#endif

#if NR_OF_DIR_SHADOW_LIGHTS
uniform DirectionalShadowLight dirShadowLights[NR_OF_DIR_SHADOW_LIGHTS];    // TODO: uniform buffer object?
uniform sampler2DShadow dirShadowMaps[NR_OF_DIR_SHADOW_LIGHTS];
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

void calcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, inout vec3 totalDiffuse, inout vec3 totalSpecular, inout vec3 totalAmbient, float shadow)
{
    // diffuse shading
    float diff = max(dot(normal, -light.direction), 0.0);

    // specular shading (Blinn-Phong)
    vec3 halfwayDir = normalize(-light.direction + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specular.a * 8.);

    // combine results
    totalAmbient += light.ambient;
    totalDiffuse += light.diffuse * diff * (1. - shadow);
    totalSpecular += light.specular * spec * (1. - shadow);
}

void calcDirShadowLight(DirectionalShadowLight light, sampler2DShadow map, vec3 normal, vec3 viewDir, inout vec3 totalDiffuse, inout vec3 totalSpecular, inout vec3 totalAmbient)
{
    float shadow = 0.;
    if (useShadows == 1)
    {
        vec4 shadowMapCoords = light.shadowSpace * vec4(v_position, 1);
        shadowMapCoords = shadowMapCoords * .5 + .5;
        if (shadowMapCoords.x >= 0. && shadowMapCoords.x <= 1. && shadowMapCoords.y >= 0. && shadowMapCoords.y <= 1.)
        {
            shadow = 1. - texture(map, shadowMapCoords.xyz);
            // OpenGL will use the Z component to compare this fragment's depth to the depth on the shadow map
            // OpenGL will return a value between 0 and 1, based on how much shadow this fragment should have.
        }
    }
    calcDirLight(light.light, normal, viewDir, totalDiffuse, totalSpecular, totalAmbient, shadow);
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
            calcDirLight(dirLights[i], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight, 0.);
    }
    #endif

    #if NR_OF_DIR_SHADOW_LIGHTS
    {   // Directional lights WITH SHADOW

        #if (NR_OF_DIR_SHADOW_LIGHTS >= 1)
        calcDirShadowLight(dirShadowLights[0], dirShadowMaps[0], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 2)
        calcDirShadowLight(dirShadowLights[1], dirShadowMaps[1], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 3)
        calcDirShadowLight(dirShadowLights[2], dirShadowMaps[2], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 4)
        calcDirShadowLight(dirShadowLights[3], dirShadowMaps[3], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
        #endif
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
