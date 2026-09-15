#version 330 core

uniform sampler2DRect uSampler;
uniform vec2 uVideoSize;

in VertexData
{
	vec2 texcoord0;
} vVertex;

out vec4 fragColor;

void main()
{
	fragColor = texture( uSampler, vVertex.texcoord0 * uVideoSize );
}
