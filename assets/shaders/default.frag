precision mediump float;

in vec3 v_position;
in vec2 v_textureCoord;

in vec3 v_tangentSunDir;
in vec3 v_tangentCamPos;
in vec3 v_tangentPosition;

out vec3 colorOut;

uniform vec3 diffuse;
uniform vec4 specular;  // 4th component is Exponent

uniform int useDiffuseTexture;
uniform sampler2D diffuseTexture;

uniform int useSpecularMap;
uniform sampler2D specularMap;

uniform int useNormalMap;
uniform sampler2D normalMap;

// PHONG
void main()
{
    vec3 normal = vec3(0, 0, 1);

    // normal map:
    if (useNormalMap == 1)
    {
        normal = texture(normalMap, v_textureCoord).xyz;
        normal = normal * 2. - 1.;
        normal = normalize(normal);   // normal needs to be normalized because the interpolation done by OpenGL changes the length of the vector
    }

    float diffuseLight = (dot(v_tangentSunDir, normal) + 1.) * .5;    // 0 - 1

    diffuseLight = max(.3, diffuseLight);   // add ambient light

    vec3 diffuseColor = diffuse;
    if (useDiffuseTexture == 1)
        diffuseColor = texture(diffuseTexture, v_textureCoord).rgb;

    colorOut = diffuseColor * diffuseLight;


    // specularity:

    vec3 viewDir = normalize(v_tangentCamPos - v_tangentPosition);
    vec3 reflectDir = reflect(-v_tangentSunDir, normal);

    float specularity = pow(max(dot(viewDir, reflectDir), 0.), specular.a);

    vec3 specularColor = specular.rgb;
    if (useSpecularMap == 1)
        specularColor = texture(specularMap, v_textureCoord).rgb;

    colorOut += specularColor * specularity;
}
