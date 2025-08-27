/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

//out vec4 FragColor;
//
//18-03-2025-->26-03-2025
out vec4 FragData[4];
//

// These definitions agree with the ObjectIds enum in scene.h
const int     nullId	= 0;
const int     skyId	= 1;
const int     seaId	= 2;
const int     groundId	= 3;
const int     roomId	= 4;
const int     boxId	= 5;
const int     frameId	= 6;
const int     lPicId	= 7;
const int     rPicId	= 8;
const int     teapotId	= 9;
const int     spheresId	= 10;
const int     floorId	= 11;

in vec3 normalVec, lightVec,eyeVec;
in vec2 texCoord;//
uniform sampler2D tex;//
uniform sampler2D normalMap;

in vec3 worldPos;
//Shadow
uniform sampler2D shadowMap;
in vec4 ShadowCoord;
vec2 shadowIndex;
float lightDepth;
float pixelDepth;
bool inShadow;
//
uniform int objectId;
uniform vec3 diffuse;

uniform vec3 specular;
uniform float shininess; 

uniform vec3 Light;    
uniform vec3 Ambient; 

const float Pi = 3.14159265;

in vec3 tanVec;

vec3 BRDF,F;
float G,D;
vec2 uv;//
vec4 color;//

bool TexturePresent = false;
bool NormalMapPresent = false;

float checker;
uniform vec3 color1; // Color of the first checker
uniform vec3 color2; // Color of the second checker
uniform float scale;  // Scale of the checkerboard
vec3 delta;

void main()
{

	vec3 N = normalize(normalVec);
    //vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);

	
	//vec3 H = normalize(L+V);
	//float LN = max(dot(L,N),0.0);
	//float HN = max(dot(H,N),0.0);

    vec3 Kd = diffuse;//color
	vec3 Ks = specular;

	vec3 Ii = Light;
	vec3 Ia = Ambient;
	float Alpha = shininess;
	
	vec3 T = normalize(tanVec);
    vec3 B = normalize(cross(T,N));

	//H = normalize(L+V);
	//LN = max(dot(L,N),0.0);
	//HN = max(dot(H,N),0.0);
	
	if(objectId == teapotId){//ok
		//texture
		uv = fract(texCoord * 4);
		TexturePresent = true;
		NormalMapPresent = false;

	}else if(objectId == boxId){//ok

		uv = fract(texCoord * 1);
		TexturePresent = true;
		NormalMapPresent = true;

	}else if(objectId == floorId){//ok
		//FLOOR DESIGN
		uv = fract(texCoord * 2);

		TexturePresent = true;
		NormalMapPresent = true;

	}else if(objectId == roomId){//ok
		//BRICKS
		//rotate the texture
		uv = fract(texCoord.yx * 20);
		TexturePresent = true;
		NormalMapPresent = true;

	}else if(objectId == groundId){//ok
		//GRASSS
		uv = fract(texCoord * 120);
		TexturePresent = true;
	
	}else if(objectId == skyId){

		uv[0] = -atan(V.y, V.x) / (2 * Pi);
		uv[1] = acos(V.z) / Pi;
		// Wrap around the skyUV.x coordinate to ensure it stays in [0, 1] range
		//uv[0] = fract(uv[0]);
		//uv = fract(uv);
		TexturePresent = true;

	}else if(objectId == seaId){

		//uv = fract(texCoord);
		uv = fract(1000*texCoord);
		TexturePresent = false;
		NormalMapPresent = true;
	
	}else if(objectId == rPicId){
		//House

		float minBound = 0.1;
		float maxBound = 0.9;

		//
		// Check if the current fragment is within the central 80% region
		if (texCoord.x < minBound || texCoord.x > maxBound || texCoord.y < minBound || texCoord.y > maxBound) {
		
			color = vec4(0.7, 0.7, 0.7, 1.0);
			Kd = color.xyz;
		
		} else {
			
			vec2 scaledTexCoord = vec2(
				(texCoord.x - minBound) / 0.8, // Scale x to fit the full texture within 80%
				(texCoord.y - minBound) / 0.8  // Scale y to fit the full texture within 80%
			);

			uv = fract(scaledTexCoord);

			color = texture(tex, uv);
			Kd = color.xyz;
		}

		TexturePresent = true;
		NormalMapPresent = false;


	}else if(objectId == lPicId) {//ok
		//Procedural texture
		// Checker board
		checker = step(0.5, mod(floor(texCoord.x * scale) + floor(texCoord.y * scale), 2.0));
		vec3 colorN = mix(color1, color2, checker);
		Kd = colorN;
		
		TexturePresent = false;
		NormalMapPresent = false;
	}

	if(TexturePresent == true && objectId != rPicId){

		color = texture(tex, uv);
		Kd = color.xyz;
	}

	if(NormalMapPresent == true){
		//NormalMap
		delta = texture(normalMap, uv).xyz;
		delta = (delta * 2.0) - vec3(1,1,1);
		N = (delta.x * T) + (delta.y * B) + (delta.z * N);
	}
	
	//reflections are to be done here 
	if(objectId == seaId){
		/*
		//vec3 R = -1.0 * (2.0 * dot(N, V) * (N - V));
	 	vec3 R = -1.0 * (2.0 * dot(N , V) * (N - V));
		uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
		uv[1] = acos(R.z) / Pi;
		//uv = clamp(uv, 0.0, 1.0);
		color = texture(tex,uv);
		*/
	}

	//
	//THE LIGHTING
	//
	//Phong Lighting Calculations
	//FragColor.xyz = vec3(0.5,0.5,0.5)*Kd + Kd*max(dot(L,N),0.0);
	//
	if(objectId != skyId && objectId != seaId){
		
	}else if(objectId == skyId || objectId == seaId) {
		//
		//Only For Sky and sea Texture
		//FragColor = color;
	}

	//Tested working
	FragData[0] = vec4(worldPos,1);//vec3(1,0,0);//R
	FragData[1] = vec4(N,1);//vec3(0,1,0);//G
	FragData[2] = vec4(Kd,1);//vec3(1,1,0);//B
	FragData[3] = vec4(Ks, Alpha);//vec3(0,0,1);

}




