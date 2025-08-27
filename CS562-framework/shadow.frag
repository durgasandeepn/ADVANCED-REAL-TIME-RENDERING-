/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

out vec4 FragColor;

in vec4 position;

uniform float z0;
uniform float z1;

void main()
{
	//
    FragColor = position;
	// FragColor = vec4(position.w)/100;
	
	/*
	//
	float depth = position.z;
	float z_normalized = (depth - z0) / (z1 - z0);
	float z2 = z_normalized * z_normalized ;
	float z3 = z2 * z_normalized ;
	float z4 = z2 * z2;
	//
	FragColor = vec4(z_normalized, z2, z3, z4);
	*/

}

