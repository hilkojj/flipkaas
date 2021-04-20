precision mediump float;

struct PointLight {
    vec3 position;

//    float constant;
//    float linear;
//    float quadratic;
    vec3 attenuation; // x: constant, y: linear, z: quadratic

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
uniform vec3 sunDirection;

#if NR_OF_POINT_LIGHTS
uniform PointLight pointLights[NR_OF_POINT_LIGHTS];    // TODO: uniform buffer object?
#endif


void calcPointLight(PointLight light, vec3 normal, vec3 viewDir, inout vec3 totalDiffuse, inout vec3 totalSpecular, inout vec3 totalAmbient)
{
    vec3 lightDir = normalize(light.position - v_position);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specular.a);

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

    float diffuseLight = 0.;//(dot(sunDirection, normal) + 1.) * .5;    // 0 - 1

    vec3 diffuseColor = diffuse;
    if (useDiffuseTexture == 1)
        diffuseColor = texture(diffuseTexture, v_textureCoord).rgb;

    colorOut = diffuseColor;

    vec3 totalDiffuseLight = vec3(diffuseLight);
    vec3 totalSpecularLight = vec3(0);
    vec3 totalAmbientLight = vec3(0);

    vec3 viewDir = normalize(camPosition - v_position);

    #if NR_OF_POINT_LIGHTS
    {   // Light points

        for (int i = 0; i < NR_OF_POINT_LIGHTS; i++)
            calcPointLight(pointLights[i], normal, viewDir, totalDiffuseLight, totalSpecularLight, totalAmbientLight);
    }
    #endif

    colorOut *= vec3(max(totalAmbientLight.r, totalDiffuseLight.r), max(totalAmbientLight.g, totalDiffuseLight.g), max(totalAmbientLight.b, totalDiffuseLight.b));

    // specularity:

//    vec3 reflectDir = reflect(-sunDirection, normal);
//    float specularity = pow(max(dot(viewDir, reflectDir), 0.), specular.a);
//    totalSpecularLight += vec3(specularity);

    vec3 specularColor = specular.rgb;
    if (useSpecularMap == 1)
        specularColor = texture(specularMap, v_textureCoord).rgb;

    colorOut += specularColor * totalSpecularLight;
}