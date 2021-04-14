layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 textureCoord;

uniform mat4 mvp;
uniform mat4 transform;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTextureCoord;

void main()
{
    gl_Position = mvp * vec4(position, 1.0);

    vNormal = vec3(transform * vec4(normal, 0.0));
    vPosition = vec3(transform * vec4(position, 1.0));
    vTextureCoord = textureCoord;
}
