/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

out vec4 FragColor;

uniform sampler2D WorldPosMap;
uniform sampler2D NMap;
uniform sampler2D KdMap;
uniform sampler2D KaMap;

in vec4 fragPos;

out vec4 FragData;

void main( )
{
		
	vec2 uv = gl_FragCoord.xy/vec2(1024,1024); // Or use textureSize
	//FragColor = texture(WorldPosMap, uv);
	//FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);

	vec3 N = texture(NMap, uv).xyz;
	vec3 P = texture(WorldPosMap, uv).xyz;

	
	
	
	float pi = 3.1415926f;
	float Value;
	vec3 Pi;

	float Scale = 2.0f;
	float K = 0.9f;
	int n = 10;
	float R = 1.0f;
	float delta = 0.001f;
	//float delta = 0.01f;
	
	float C = 0.1 * R;



	ivec2 intCoeff = ivec2(gl_FragCoord.xy);
	vec2 floatCoeff = vec2( intCoeff.x/1024.0f, intCoeff.y/1024.0f );
	float CameraDepth = texture(WorldPosMap, uv).a;
	float Phi = (30 * (intCoeff.x ^ intCoeff.y)) + (10 * (intCoeff.x * intCoeff.y));


	for(int i = 0; i < n; i++){
		
		float alpha = (i + 0.5) / n;
		float h = (alpha * R) / CameraDepth;
		float theta = (2 * pi * alpha) * ( 7.0f * n / 9.0f ) + Phi;
		
		Pi = texture(WorldPosMap, floatCoeff + h * vec2(cos(theta), sin(theta))).xyz;
		float d_i = texture(WorldPosMap, floatCoeff + h * vec2(cos(theta), sin(theta))).a;
		
		vec3 Omega_i = (Pi - P);
		
		Value += (max(0,dot(N,Omega_i) - (delta * d_i)) * 
		step( 0.0f, R - length(Omega_i)))/max((C * C), dot(Omega_i, Omega_i));
	}

	float S = ((2 * pi * C) / n) * Value;



	float A = pow(max(0.0f,(1 - Scale * S)), K);
	
	//
	//FragData = vec4(fragPos.xyz, 1.0);
	//FragData = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	FragData = vec4(vec3(A), 1.0f);
	//
	return;
	//
}








 





