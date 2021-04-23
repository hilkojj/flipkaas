
precision mediump float;

out vec3 result;

in vec2 v_texCoords;

uniform sampler2D image;
uniform bool horizontal;

void main()
{
    const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    vec2 texOffset = 1.0 / vec2(textureSize(image, 0)); // gets size of single texel
    result = texture(image, v_texCoords).rgb * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, v_texCoords + vec2(texOffset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, v_texCoords - vec2(texOffset.x * float(i), 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, v_texCoords + vec2(0.0, texOffset.y * float(i))).rgb * weight[i];
            result += texture(image, v_texCoords - vec2(0.0, texOffset.y * float(i))).rgb * weight[i];
        }
    }
}
