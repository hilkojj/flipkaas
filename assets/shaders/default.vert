layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_textureCoord;

uniform mat4 mvp;
uniform mat4 transform;

uniform vec3 camPosition;
uniform vec3 sunDirection;

out vec3 v_position;
out vec2 v_textureCoord;

out vec3 v_tangentSunDir;
out vec3 v_tangentCamPos;
out vec3 v_tangentPosition;


void main()
{
    gl_Position = mvp * vec4(a_position, 1.0);

    v_position = vec3(transform * vec4(a_position, 1.0));
    v_textureCoord = a_textureCoord;

    mat3 normalMatrix = transpose(inverse(mat3(transform)));

    vec3 normal = normalMatrix * a_normal;
    vec3 tangent = normalMatrix * a_tangent;
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitan = normalize(cross(normal, tangent)); // todo, is normalize needed?

    mat3 TBN = transpose(mat3(tangent, bitan, normal));

    v_tangentSunDir = TBN * sunDirection;
    v_tangentCamPos = TBN * camPosition;
    v_tangentPosition = TBN * v_position;
}
