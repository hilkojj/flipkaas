precision mediump float;
precision mediump sampler2DShadow;

#define PI 3.14159265359


struct PointLight
{
    vec3 position;

//    float constant;
//    float linear;
//    float quadratic;
    vec3 attenuation; // x: constant, y: linear, z: quadratic

    vec3 color;
};

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
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

#if FOG
in float v_fog;
#endif

layout (location = 0) out vec4 colorOut;
#if BLOOM
layout (location = 1) out vec4 brightColor;
#endif

uniform float time;

uniform vec3 diffuse;
uniform vec2 metallicRoughnessFactors;

uniform int useDiffuseTexture;
uniform sampler2D diffuseTexture;

uniform int useMetallicRoughnessTexture;
uniform sampler2D metallicRoughnessTexture;

uniform int useNormalMap;
uniform sampler2D normalMap;

uniform int useShadows; // todo: different shader for models that dont receive shadows? Sampling shadowmaps is expensive

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

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


// ----------------- PBR ----------------------
/*
Physically Based Rendering.

Thanks to:
https://learnopengl.com/PBR/Theory
https://learnopengl.com/PBR/Lighting
*/

/*
fresnelSchlick()

"calculate the ratio between specular and diffuse reflection"
"or how much the surface reflects light versus how much it refracts light"

cosTheta: probably the dot product of halfway vector and view direction

F0: "how much the surface reflects if looking directly at the surface"
    tinted if the material is metallic, white/grey otherwise.
*/
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// normal distribution function D
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness; // "Based on observations by Disney and adopted by Epic Games, the lighting looks more correct squaring the roughness in both the geometry and normal distribution function."
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.) + 1.);
    denom = PI * denom * denom;

    return num / denom;
}

// geometry function G
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.;
    float k = (r * r) / 8.;

    float num = NdotV;
    float denom = NdotV * (1. - k) + k;

    return num / denom;
}
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.);
    float NdotL = max(dot(N, L), 0.);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


// calculate per-light radiance
void pointLightRadiance(PointLight light, vec3 N, vec3 V, vec3 F0, inout vec3 Lo, float roughness, float metallic, vec3 albedo)
{
    vec3 L = normalize(light.position - v_position);
    vec3 H = normalize(V + L);  // halfway
    float distance = length(light.position - v_position);

    // attenuation
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;

    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
//    float attenuation = 1.0 / (distance * distance);

    vec3 radiance = light.color * attenuation;

    // cook-torrance brdf
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = numerator / max(denominator, 0.001);

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
}

void dirLightRadiance(DirectionalLight light, vec3 N, vec3 V, vec3 F0, inout vec3 Lo, float roughness, float metallic, vec3 albedo, float shadow)
{
    vec3 L = -light.direction;
    vec3 H = normalize(V + L);  // halfway

    vec3 radiance = light.color * (1. - shadow);

    // cook-torrance brdf
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular     = numerator / max(denominator, 0.001);

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
}

void dirShadowLightRadiance(DirectionalShadowLight light, vec3 N, vec3 V, vec3 F0, inout vec3 Lo, float roughness, float metallic, vec3 albedo, sampler2DShadow map, inout float fakeShadow)
{
    float shadow = 0.;
    if (useShadows == 1)
    {
        vec3 biasedPos = v_position + N * .05;   // https://digitalrune.github.io/DigitalRune-Documentation/html/3f4d959e-9c98-4a97-8d85-7a73c26145d7.htm
        vec4 shadowMapCoords = light.shadowSpace * vec4(biasedPos, 1);
        shadowMapCoords = shadowMapCoords * .5 + .5;
        if (shadowMapCoords.x >= 0. && shadowMapCoords.x <= 1. && shadowMapCoords.y >= 0. && shadowMapCoords.y <= 1. && shadowMapCoords.z <= 1.)
        {
            shadow = 1. - texture(map, shadowMapCoords.xyz);
            // OpenGL will use the Z component to compare this fragment's depth to the depth on the shadow map
            // OpenGL will return a value between 0 and 1, based on how much shadow this fragment should have.

            if (light.light.color.x < 0.f)
            {
                shadow *= 1.f - shadowMapCoords.z;
                fakeShadow += shadow * -light.light.color.x;
                return;
            }
        }
    }
    if (light.light.color.x < 0.f)
    {
        return;
    }

    dirLightRadiance(light.light, N, V, F0, Lo, roughness, metallic, albedo, shadow);
}


// --------------------------------------------

void main()
{
    vec3 albedo = diffuse;
    if (useDiffuseTexture == 1)
    {
        vec4 albedoAlpha = texture(diffuseTexture, v_textureCoord);
        albedo = albedoAlpha.rgb;
        if (albedoAlpha.a < .5)
            discard;
        albedo = pow(albedo, vec3(GAMMA)); // sRGB to linear space. https://learnopengl.com/Advanced-Lighting/Gamma-Correction
    }

    float metallic = metallicRoughnessFactors.x;
    float roughness = metallicRoughnessFactors.y;
    if (useMetallicRoughnessTexture == 1)
    {
        vec2 mr = texture(metallicRoughnessTexture, v_textureCoord).xy;
        metallic = mr.x;
        roughness = mr.y;
    }

    float ao = 1.;

    vec3 N = vec3(0, 0, 1);    // normal will be in World space.

    // normal map:
    if (useNormalMap == 1)
    {
        N = texture(normalMap, v_textureCoord).xyz;
        N = N * 2. - 1.;
    }
    N = normalize(v_TBN * N);

    vec3 V = normalize(camPosition - v_position);  // View vector

    vec3 F0 = vec3(0.04);                   // "surface reflection at zero incidence"
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.);

    #if NR_OF_POINT_LIGHTS
    {   // Light points

        for (int i = 0; i < NR_OF_POINT_LIGHTS; i++)
            pointLightRadiance(pointLights[i], N, V, F0, Lo, roughness, metallic, albedo);
    }
    #endif

    #if NR_OF_DIR_LIGHTS
    {   // Directional lights without shadow

        for (int i = 0; i < NR_OF_DIR_LIGHTS; i++)
            dirLightRadiance(dirLights[i], N, V, F0, Lo, roughness, metallic, albedo, 0.f);
    }
    #endif

    float fakeShadow = 0.f;

    #if NR_OF_DIR_SHADOW_LIGHTS
    {   // Directional lights WITH SHADOW

        #if (NR_OF_DIR_SHADOW_LIGHTS >= 1)
        dirShadowLightRadiance(dirShadowLights[0], N, V, F0, Lo, roughness, metallic, albedo, dirShadowMaps[0], fakeShadow);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 2)
        dirShadowLightRadiance(dirShadowLights[1], N, V, F0, Lo, roughness, metallic, albedo, dirShadowMaps[1], fakeShadow);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 3)
        dirShadowLightRadiance(dirShadowLights[2], N, V, F0, Lo, roughness, metallic, albedo, dirShadowMaps[2], fakeShadow);
        #endif
        #if (NR_OF_DIR_SHADOW_LIGHTS >= 4)
        dirShadowLightRadiance(dirShadowLights[3], N, V, F0, Lo, roughness, metallic, albedo, dirShadowMaps[3], fakeShadow);
        #endif
    }
    #endif

    vec3 R = reflect(-V, N);
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1. - kS;
    kD *= 1. - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseColor = irradiance * albedo;

    // TODO: ldjam hack
    diffuseColor += vec3(pow(1.f - dot(N, V), 5.f)) * (albedo + vec3(1.f));

    const float MAX_REFLECTION_LOD = 4.;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularColor = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuseColor + specularColor) * ao;

    vec3 color = ambient + Lo;
    color *= 1. - fakeShadow;

    // gamma correction:
    colorOut.rgb = pow(color, vec3(1.0 / GAMMA));

    #if FOG
    // fog:
    colorOut.a = v_fog;

    #ifdef FOG_BASED_ON_HEIGHT
    
    colorOut.a = max(min(v_fog, (v_position.y + 70.f)/100.f), 0.f);

    #endif

    #else
    colorOut.a = 1.;
    #endif

    #if BLOOM
    // check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(colorOut.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > BLOOM_THRESHOLD)
        brightColor.rgb = colorOut.rgb;
    else
        brightColor.rgb = vec3(0);

    brightColor.a = colorOut.a;

    #endif
}

