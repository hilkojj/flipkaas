precision mediump float;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTextureCoord;

uniform vec3 camPosition;
uniform vec3 sunDirection;
uniform vec3 diffuse;
uniform vec4 specular;  // 4th component is Exponent
uniform int useTexture;
uniform sampler2D diffuseTexture;

// PHONG
void main()
{
    float diffuseLight = (dot(sunDirection, vNormal) + 1.) * .5;    // 0 - 1

    diffuseLight = max(.3, diffuseLight);   // add ambient light

    vec3 diffuseColor = diffuse;
    if (useTexture == 1)
    diffuseColor = texture(diffuseTexture, vTextureCoord).rgb;

    gl_FragColor = vec4(diffuseColor * diffuseLight, 1.0);


    // specularity:

    vec3 viewDir = normalize(camPosition - vPosition);
    vec3 reflectDir = reflect(-sunDirection, normalize(vNormal));   // vNormal needs to be normalized because the interpolation done by OpenGL changes the length of the vector

    float specularity = pow(max(dot(viewDir, reflectDir), 0.), specular.a);

    gl_FragColor.rgb += specular.rgb * specularity;
}
