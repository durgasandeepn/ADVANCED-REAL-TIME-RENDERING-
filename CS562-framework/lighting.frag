/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 430

out vec4 FragColor;

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

//in vec3 normalVec, lightVec,eyeVec;
//in vec2 texCoord;//
uniform sampler2D tex;//
uniform sampler2D normalMap;

//Shadow
//uniform sampler2DShadow shadowMap;
uniform sampler2D shadowMap;
vec4 ShadowCoord;
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
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);//

bool TexturePresent = false;
bool NormalMapPresent = false;

float checker;
uniform vec3 color1; // Color of the first checker
uniform vec3 color2; // Color of the second checker
uniform float scale; // Scale of the checkerboard
vec3 delta;

vec2 PixelPos;

//
uniform sampler2D BlurredShadowMap;

//	
//uniform vec3 lightPos1;
//	
uniform sampler2D WorldPosMap;
uniform sampler2D NMap;
uniform sampler2D KdMap;
uniform sampler2D KaMap;

uniform mat4 WorldInverse,ShadowMatrix;//WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMatrix;

uniform vec3 lightPos1;//LIGHT POSITION
vec3 eyePos;
vec3 normalVec, lightVec, eyeVec;
in vec2 texCoord;//06-04-2025 - needed

vec3 uv3;
vec3 WorldPos_Sha;//for shadow

void main()
{	

	/*
	vec2 uv = gl_FragCoord.xy/vec2(750,750); // Or use textureSize
    vec3 pos = texture(WorldPosMap, uv).xyz;

    // Visualize the position (scale/bias might be needed to see colors)
    FragColor = vec4(abs(pos) * 0.1, 1.0); // Example visualization

	return;
	*/

	//
	//texCoord
	uv = gl_FragCoord.xy/vec2(750,750);
	//
	WorldPos_Sha = texture(WorldPosMap, uv).xyz;
	ShadowCoord = ShadowMatrix * vec4(WorldPos_Sha, 1.0);

	//PixelPos = gl_FragColor.xy/vec2(750,750);
	//EYE POSITION
	eyePos = (WorldInverse*vec4(0,0,0,1)).xyz;
	uv3 = vec3(uv, 0.0);
	eyeVec = eyePos - uv3;// texture(WorldPosMap, uv).xyz;
	//
	lightVec = lightPos1 - texture(WorldPosMap, uv).rgb;//light
	//


	//
	//normalVec = ;
	//lightVec = ;
	//

	//
	//TESTING--->TESTED WORKING
	//uv = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
	//FragColor.xyz = abs(texture(NMap, uv).xyz);//working
	//FragColor.xyz = (texture(WorldPosMap, uv).xyz)/2;//working
	//FragColor.xyz = (texture(KdMap, uv).xyz);//working
	//FragColor.xyz = (texture(KaMap, uv).xyz) * 50;//working
	//return;

	/*
	uv = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
	FragColor.xyz = vec3(texture(shadowMap, uv).w/100.0); // or similar
	FragColor.w = 1.0;
	return; // which disables all further code in the shader
	*/

	vec3 N = texture(NMap, uv).rgb;//changed
    vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);//done
	
	vec3 H = normalize(L+V);
	float LN = max(dot(L,N),0.0);
	float HN = max(dot(H,N),0.0);

    //vec3 Kd = diffuse;//color texture(WorldPosMap, uv).xyz;
    vec3 Kd = texture(KdMap, uv).xyz;
	vec3 Ks = texture(KaMap, uv).xyz;
	
	vec3 Ii = Light;
	vec3 Ia = Ambient;
	float Alpha = texture(KaMap, uv).a;
	
	vec3 T = normalize(tanVec);
    vec3 B = normalize(cross(T,N));

	shadowIndex = ShadowCoord.xy/ShadowCoord.w;

	if (shadowIndex.x >= 0.0 && shadowIndex.x <= 1.0 &&
        shadowIndex.y >= 0.0 && shadowIndex.y <= 1.0 &&
        ShadowCoord.w > 0.0)
    {
        // Sample the shadow map (depth value from light's perspective)
        lightDepth = texture2D(shadowMap, shadowIndex).w;
        pixelDepth = ShadowCoord.w - 0.05;
		
        // Determine if the fragment is in shadow (if pixelDepth > lightDepth)
		inShadow =  (pixelDepth > lightDepth);
	
    }
    else
    {
        inShadow = false;  // Default to being in shadow if out of bounds
    }


	//inShadow = false;
	
	//
	//
    // A checkerboard pattern to break up large flat expanses.  Remove when using textures.
	/*
    if (objectId==groundId || objectId==floorId || objectId==seaId) {
        ivec2 uv = ivec2(floor(100.0*texCoord));
        if ((uv[0]+uv[1])%2==0)
            Kd *= 0.9; }
	*/
	//
	//
	

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
		// Procedural texture
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
		//delta = texture(normalMap, uv).xyz;
		//delta = (delta * 2.0) - vec3(1,1,1);
		//N = (delta.x * T) + (delta.y * B) + (delta.z * N);

	}

	H = normalize(L+V);
	LN = max(dot(L,N),0.0);
	HN = max(dot(H,N),0.0);

	//
	//Normal Calculations are done above 
	//reflections are to be done here 
	if(objectId == seaId){
		/*
	 	vec3 R = -1.0 * (2.0 * dot(N , V) * (N - V));
		uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
		uv[1] = acos(R.z) / Pi;
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
	
		D =	((Alpha + 2.0)/(2.0*Pi)) * pow((HN),Alpha);
		F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,L),0.0)),5);
		G = (1.0/(pow(max(dot(H,L),0.00001),2)));
	
		BRDF = (Kd/Pi) + (F * G * D) / 4.0;

		//FragColor.xyz = (Ia * Kd) + Ii*(LN)	*(BRDF * color.xyz);
		
		//
		// If in shadow, only apply ambient lighting
        if (inShadow == true) 
		{
			//In Shadow
			FragColor.xyz = (Ia * Kd);  // Ambient lighting only

		} else if (inShadow == false) 
		{
			//In lighting
			FragColor.xyz = (Ia * Kd) + Ii*(LN)	* (BRDF);
		}

		
		FragColor.w = 1.0;
		
	}else if(objectId == skyId || objectId == seaId) {
		//
		//Only For Sky and sea Texture
		FragColor = color;
	}


}




