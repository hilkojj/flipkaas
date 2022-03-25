layout(location = 0) in vec3 a_position;

#ifdef INSTANCED

layout(location = 4) in mat4 transform;
uniform mat4 viewProjection;

#else
uniform mat4 mvp;
#endif

void main()
{
    #ifdef INSTANCED

    mat4 mvp = viewProjection * transform;

    #endif

    gl_Position = mvp * vec4(a_position, 1.0);
}
