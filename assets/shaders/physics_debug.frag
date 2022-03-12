precision mediump float;

layout (location = 0) out vec4 colorOut;

in vec4 v_color;

void main()
{
	colorOut = v_color;
}
