/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330

out vec4 FragColor;

in vec4 position;

uniform float z0;//z_near
uniform float z1;//z_far


void main()
{
	/*
    FragColor = position ;
//  FragColor = vec4(position.w)/100;
	*/
	
	float depth = position.w;
	//float depth = position.z;
	float z_normalized = (depth - 0.1f) / (100.0f - 0.1f);
	float z2 = z_normalized * z_normalized ;
	float z3 = z2 * z_normalized ;
	float z4 = z2 * z2;
	FragColor = vec4(z_normalized, z2, z3, z4);

	//FragColor = position ;

}

