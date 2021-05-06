precision mediump float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_textureCoord;

uniform mat4 mvp;
uniform mat4 transform;

uniform vec3 camPosition;

out vec3 v_position;
out vec2 v_textureCoord;
out mat3 v_TBN;
#if FOG
out float v_fog;
#endif

void main()
{
    gl_Position = mvp * vec4(a_position, 1.0);

    v_position = vec3(transform * vec4(a_position, 1.0));
    v_textureCoord = a_textureCoord;

    mat3 dirTrans = mat3(transform);

    vec3 normal = normalize(dirTrans * a_normal);
    vec3 tangent = normalize(dirTrans * a_tangent);
    vec3 bitan = normalize(cross(normal, tangent)); // todo, is normalize needed?

    v_TBN = mat3(tangent, bitan, normal);
    #if FOG
    v_fog = 1. - max(0., min(1., (length(v_position - camPosition) - FOG_START) / (FOG_END - FOG_START)));
    #endif
}
