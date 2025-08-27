/////////////////////////////////////////////////////////////////////////
// Vertex shader for SHADOW
////////////////////////////////////////////////////////////////////////
#version 430

uniform mat4  L_ViewMatrix, L_ProjectionMatrix, ModelTr;

in vec4 vertex;

out vec4 position;

void main()
{   
	gl_Position = L_ProjectionMatrix * L_ViewMatrix * ModelTr * vertex;
	position = gl_Position;
}

