/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

//out vec4 FragColor;

//vec2 uv;//For Test only // we are calculating uv in the shader itself.

struct PointLight {
        vec3 position;
        vec3 color;
        vec3 intensity;
        float radius;
};

const int MAX_POINT_LIGHTS = 1000;//1000;//4;//1000;
//uniform PointLight pointlights[MAX_POINT_LIGHTS];

//
//Textures
uniform sampler2D WorldPosMap;
uniform sampler2D NMap;
uniform sampler2D KdMap;
uniform sampler2D KaMap;


layout(std430, binding = 1) buffer TextureBuffers{
	PointLight pointlights[MAX_POINT_LIGHTS];
	//vec4 textureDataIndices[4];
};

uniform int lightIndex;
// Inputs from the vertex shader
//in vec2 uv;                // Texture coordinates passed from the vertex shader
out vec4 fragColor;        // Output color of the fragment

//float dist = 0;

vec3 BRDF,F;
float G,D;

uniform vec3 lightPos1;//LIGHT POSITION
//vec3 eyePos;
vec3 normalVec, lightVec;
in vec3 eyeVec;

uniform mat4 WorldInverse;
vec3 uv3;
//in vec3 tanVec;

const float Pi = 3.14159265;

vec3 Ii;
vec3 Ia;


void main( ){
	
	//
	//uv1 = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
	//fragColor.xyz = abs(texture(NMap, uv).xyz);//working
	//fragColor.xyz = (texture(WorldPosMap, uv).xyz)/2;//working
	//fragColor.xyz = (texture(KdMap, uv).xyz);//working
	//fragColor.xyz = (texture(KaMap, uv).xyz) * 50;//working
	//return;
	//

    vec2 uv = gl_FragCoord.xy/vec2(750,750);
    vec3 fragPosition = texture(WorldPosMap, uv).xyz;
    float dist = length(pointlights[lightIndex].position - texture(WorldPosMap, uv).xyz);

    if(dist > pointlights[lightIndex].radius){
        //it is outside range
        //fragColor = vec4(0,0,0,0);//black color
		discard;
		
    }else{ // inside radius 
		
		//
		//USING THE BOTTOM ONE
		float attenuation = max(0.0, (1/(dist * dist)) - ( 1 / (pointlights[lightIndex].radius * pointlights[lightIndex].radius)) );
		//float attenuation = 1;

		Ii = pointlights[lightIndex].color * pointlights[lightIndex].intensity;
        
		lightVec = pointlights[lightIndex].position - fragPosition.xyz;
		//
		vec3 N = texture(NMap, uv).rgb;//changed
		vec3 L = normalize(lightVec);
		vec3 V = normalize(eyeVec);//done
		
		vec3 H = normalize(L+V);	
		float LN = max(dot(L,N),0.0);
		float HN = max(dot(H,N),0.0);

		vec3 Kd = texture(KdMap, uv).xyz;
		vec3 Ks = texture(KaMap, uv).xyz;

		float Alpha = texture(KaMap, uv).a;
	
		//vec3 T = normalize(tanVec);
		//vec3 B = normalize(cross(T,N));

		H = normalize(L+V);
		LN = max(dot(L,N),0.0);
		HN = max(dot(H,N),0.0);

		D =	((Alpha + 2.0)/(2.0*Pi)) * pow((HN),Alpha);
		//F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,L),0.0)),5);
		F = Ks + (vec3(1.0) - Ks) * pow((1 - max(dot(H,L),0.0)),5);
		G = (1.0/(pow(max(dot(H,L),0.00001),2)));
		
		BRDF = (Kd/Pi) + (F * G * D) / 4.0;
		
		vec3 lightContribution = attenuation * Ii * LN * (BRDF);// * Ia * LN * (BRDF);
		//vec3 lightContribution = abs(N);

		fragColor.xyz += lightContribution;
		fragColor.a = 1.0;
    }

	//fragColor.xyz = vec3(1,0,0);
    //discard;//this will not render the sphere
}








