layout(location = 0) in vec3 a_position;

layout(location = 4) in uvec4 a_boneIds;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mvp;

uniform mat4 bonePoseTransforms[MAX_BONES];

void applyBone(uint boneId, float weight, inout vec4 totalLocalPos, inout float w)
{
    w += weight;

    mat4 poseTransform = bonePoseTransforms[boneId];

    vec4 posePosition = poseTransform * vec4(a_position, 1.);
    totalLocalPos += posePosition * weight;
}

void main()
{
    float weight = 0.;
    vec4 totalLocalPos = vec4(0.0);

    applyBone(a_boneIds[0], a_boneWeights[0], totalLocalPos, weight);
    applyBone(a_boneIds[1], a_boneWeights[1], totalLocalPos, weight);
    applyBone(a_boneIds[2], a_boneWeights[2], totalLocalPos, weight);
    applyBone(a_boneIds[3], a_boneWeights[3], totalLocalPos, weight);

    totalLocalPos.xyz += a_position * (1. - weight);

    gl_Position = mvp * vec4(totalLocalPos.xyz, 1.0);
}
