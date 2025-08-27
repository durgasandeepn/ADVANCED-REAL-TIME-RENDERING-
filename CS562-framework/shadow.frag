/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

out vec4 FragColor;

in vec4 position;

uniform float z0;//z_near
uniform float z1;//z_far

void main()
{
	/*
	float depth = position.z;
	///FragColor = vec4(depth, depth, depth, 1.0);
	FragColor = vec4(0, depth*depth, 0, 1.0);
	return;
	*/

	float depth = position.z;
	float z_normalized = depth;
	float z2 = z_normalized * z_normalized ;
	float z3 = z2 * z_normalized ;
	float z4 = z2 * z2;
	FragColor = vec4(z_normalized, z2, z3, z4);//this is the main code line

}

