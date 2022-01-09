precision mediump float;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_textureCoord;

layout(location = 4) in uvec4 a_boneIds;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mvp;
uniform mat4 transform;

uniform vec3 camPosition;

uniform mat4 bonePoseTransforms[MAX_BONES];

out vec3 v_position;
out vec2 v_textureCoord;
out mat3 v_TBN;
#if FOG
out float v_fog;
#endif

void applyBone(uint boneId, float weight, inout vec4 totalLocalPos, inout vec4 totalNormal, inout vec4 totalTangent, inout float w)
{
    w += weight;

    mat4 poseTransform = bonePoseTransforms[boneId];

    vec4 posePosition = poseTransform * vec4(a_position, 1.);
    totalLocalPos += posePosition * weight;

    vec4 poseNormal = poseTransform * vec4(a_normal, .0);
    totalNormal += poseNormal * weight;

    vec4 poseTangent = poseTransform * vec4(a_tangent, .0);
    totalTangent += poseTangent * weight;
}

void main()
{
    float weight = 0.;
    vec4 totalLocalPos = vec4(0.0);
    vec4 totalNormal = vec4(0.0);
    vec4 totalTangent = vec4(0.0);

    applyBone(a_boneIds[0], a_boneWeights[0], totalLocalPos, totalNormal, totalTangent, weight);
    applyBone(a_boneIds[1], a_boneWeights[1], totalLocalPos, totalNormal, totalTangent, weight);
    applyBone(a_boneIds[2], a_boneWeights[2], totalLocalPos, totalNormal, totalTangent, weight);
    applyBone(a_boneIds[3], a_boneWeights[3], totalLocalPos, totalNormal, totalTangent, weight);

    gl_Position = mvp * vec4(totalLocalPos.xyz, 1.0);

    v_position = vec3(transform * vec4(totalLocalPos.xyz, 1.0));
    v_textureCoord = a_textureCoord;

    mat3 dirTrans = mat3(transform);

    vec3 normal = normalize(dirTrans * totalNormal.xyz);
    vec3 tangent = normalize(dirTrans * totalTangent.xyz);
    vec3 bitan = normalize(cross(normal, tangent)); // todo, is normalize needed?

    v_TBN = mat3(tangent, bitan, normal);

    #if FOG
    v_fog = 1. - max(0., min(1., (length(v_position - camPosition) - FOG_START) / (FOG_END - FOG_START)));
    #endif
}
