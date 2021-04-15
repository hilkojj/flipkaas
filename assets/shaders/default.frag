precision mediump float;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTextureCoord;

out vec3 colorOut;

uniform vec3 camPosition;
uniform vec3 sunDirection;
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
    float diffuseLight = (dot(sunDirection, vNormal) + 1.) * .5;    // 0 - 1

    diffuseLight = max(.3, diffuseLight);   // add ambient light

    vec3 diffuseColor = diffuse;
    if (useDiffuseTexture == 1)
        diffuseColor = texture(diffuseTexture, vTextureCoord).rgb;

    colorOut = diffuseColor * diffuseLight;


    // specularity:

    vec3 viewDir = normalize(camPosition - vPosition);
    vec3 reflectDir = reflect(-sunDirection, normalize(vNormal));   // vNormal needs to be normalized because the interpolation done by OpenGL changes the length of the vector

    float specularity = pow(max(dot(viewDir, reflectDir), 0.), specular.a);

    vec3 specularColor = specular.rgb;
    if (useSpecularMap == 1)
        specularColor = texture(specularMap, vTextureCoord).rgb;

    colorOut += specularColor * specularity;
}
