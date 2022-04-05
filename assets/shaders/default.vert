precision mediump float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_textureCoord;

#ifdef INSTANCED

layout(location = 4) in mat4 transform;
uniform mat4 viewProjection;

#else
uniform mat4 mvp;
uniform mat4 transform;
#endif

uniform vec3 camPosition;

out vec3 v_position;
out vec2 v_textureCoord;
out mat3 v_TBN;
#if FOG
out float v_fog;
#endif

#ifdef SHINY
uniform float time;
#endif

#ifdef APPLE
uniform float time;
out vec3 v_modelPosition;
#endif
#ifdef TITLE
uniform float time;
#endif

void main()
{
    #ifdef INSTANCED

    mat4 mvp = viewProjection * transform;

    #endif

    vec3 position = a_position;

    #ifdef SHINY

    position.y += sin(time * 2.f) * .2f;

    #endif

    #ifdef APPLE

    float xInfluence = clamp(position.x / 10.f, 0.f, 1.f);
    if (position.y > 19.f)
    {
        position.y += sin(time * -1.5f + position.x * 1.f) * xInfluence * .4f;

    }
    v_modelPosition = vec3(transform * vec4(vec3(0), 1.0));


    #endif

    gl_Position = mvp * vec4(position, 1.0);

    v_position = vec3(transform * vec4(position, 1.0));
    v_textureCoord = a_textureCoord;

    mat3 dirTrans = mat3(transform);

    vec3 normal = normalize(dirTrans * a_normal);
    vec3 tangent = normalize(dirTrans * a_tangent);
    vec3 bitan = normalize(cross(normal, tangent)); // todo, is normalize needed?

    v_TBN = mat3(tangent, bitan, normal);
    #if FOG
    v_fog = 1. - max(0., min(1., (length(v_position - camPosition) - FOG_START) / (FOG_END - FOG_START)));

    #ifdef TITLE
    v_fog = clamp(((position.z * 30.f + 50.f) / 30.f), 0.f, 1.f);
    #endif

    #endif
}
