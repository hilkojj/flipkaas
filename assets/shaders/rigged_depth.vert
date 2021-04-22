layout(location = 0) in vec3 a_position;

layout(location = 4) in vec2 a_boneIdAndWeight0;
layout(location = 5) in vec2 a_boneIdAndWeight1;
layout(location = 6) in vec2 a_boneIdAndWeight2;
layout(location = 7) in vec2 a_boneIdAndWeight3;

uniform mat4 mvp;

uniform mat4 bonePoseTransforms[MAX_BONES];

void applyBone(vec2 boneIdAndWeight, inout vec4 totalLocalPos, inout float w)
{
    int boneId = int(boneIdAndWeight.x);
    float weight = boneIdAndWeight.y;

    w += weight;

    mat4 poseTransform = bonePoseTransforms[boneId];

    vec4 posePosition = poseTransform * vec4(a_position, 1.);
    totalLocalPos += posePosition * weight;
}

void main()
{
    float weight = 0.;
    vec4 totalLocalPos = vec4(0.0);

    applyBone(a_boneIdAndWeight0, totalLocalPos, weight);
    applyBone(a_boneIdAndWeight1, totalLocalPos, weight);
    applyBone(a_boneIdAndWeight2, totalLocalPos, weight);
    applyBone(a_boneIdAndWeight3, totalLocalPos, weight);

    totalLocalPos.xyz += a_position * (1. - weight);

    gl_Position = mvp * vec4(totalLocalPos.xyz, 1.0);
}
