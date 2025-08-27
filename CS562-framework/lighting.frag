
/////////////////////////////////////////////////////////////////////////
// Pixel shader for lighting
////////////////////////////////////////////////////////////////////////
#version 330
//
out vec4 FragColor;
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

//Shadow
//uniform sampler2DShadow shadowMap;
uniform sampler2D shadowMap;
in vec4 ShadowCoord;
vec2 shadowIndex;
float lightDepth;
float pixelDepth;
bool inShadow;
//
//Reflection
uniform sampler2D UpMap,LowerMap;
//
//Irradiance
uniform sampler2D IrradianceMap;
uniform sampler2D SkyMapHDR;
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

bool TexturePresent;
bool NormalMapPresent;

float checker;
uniform vec3 color1; // Color of the first checker
uniform vec3 color2; // Color of the second checker
uniform float scale;  // Scale of the checkerboard
vec3 delta;

//
//Reflection

//
//IBL
vec3 irradianceBRDF;
vec3 r;
vec3 R;
float e = 2.5;
//uniform float e;
vec3 C;

void main()
{
	 
	/*	vec2 uv = gl_FragCoord.xy/vec2(1024,1024); // (or whatever screen size)
	FragColor.xyz = vec3(texture(LowerMap, uv)); // or similar
	//FragColor.xyz = vec3(texture(UpMap, uv)); // or similar
	return; // which disables all further code in the shader.
	*/

	vec3 N = normalize(normalVec);
    vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);
	
	vec3 H = normalize(L+V);
	float LN = max(dot(L,N),0.0);
	float HN = max(dot(H,N),0.0);

    vec3 Kd = diffuse;//color
	vec3 Ks = specular;

	vec3 Ii = Light;
	vec3 Ia = Ambient;
	float Alpha = shininess;
	
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
		
		//
        // Determine if the fragment is in shadow (if pixelDepth > lightDepth)
		inShadow =  (pixelDepth > lightDepth);
		//
    }
    else
    {
        inShadow = false;  // Default to being in shadow if out of bounds
    }


	
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
		delta = texture(normalMap, uv).xyz;
		delta = (delta * 2.0) - vec3(1,1,1);
		N = (delta.x * T) + (delta.y * B) + (delta.z * N);
	}

	H = normalize(L+V);
	LN = max(dot(L,N),0.0);
	HN = max(dot(H,N),0.0);

	//
	//Normal Calculations are to be  done above
	//
	//Reflections are to be done here 
	//Reflection
	if(objectId == teapotId)
	{
		
		float c;
		
		//Reflection
		r = 2 * (dot(V,N)) * N - V;
		r = normalize(r);
		c = r.z;

		if(c > 0){
			
			uv = vec2( r.x/( 1 + c), r.y/( 1 + c) ) * 0.5 + vec2(0.5,0.5);
			color =  texture(UpMap, uv);
			//FragColor.xyz = Kd;
			//FragColor.w = 1.0;
			//return;

		}else {
			
			uv = vec2( r.x/( 1 - c), r.y/( 1 - c) ) * 0.5 + vec2(0.5,0.5);
			color =  texture(LowerMap, uv);
			//FragColor.xyz = Kd;
			//FragColor.w = 1.0;
			//return;

		}

		Kd = color.xyz;
		
	}


	if(objectId == seaId){

	 	R = -1.0 * (2.0 * dot(N , V) * (N - V));
		R  = normalize(R);
		uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
		uv[1] = acos(R.z) / Pi;
		color = texture(tex,uv);

	}

	//
	//THE LIGHTING
	//
	//Phong Lighting Calculations
	//FragColor.xyz = vec3(0.5,0.5,0.5)*Kd + Kd*max(dot(L,N),0.0);
	if(objectId != skyId && objectId != seaId){

			uv[0] = -atan(-N.y, -N.x) / (2 * Pi);
			uv[1] = acos(N.z) / Pi;

			R = -1.0 * (2.0 * dot(N , V) * N - V);
			
			if(objectId != teapotId){
				e = 2.5;
				//Kd = pow(Kd , vec3(2.2)); 
				Kd = pow((e*Kd) / (e * Kd + vec3(1,1,1)), vec3(2.2));
			}
			
			irradianceBRDF = texture(IrradianceMap, uv).xyz * Kd/Pi * 10;

			//teapot
			//gorund
			//box

			/*
			D =	((Alpha + 2.0)/(2.0*Pi)) * pow((HN),Alpha);
			F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,L),0.0)),5);
			G = (1.0/(pow(max(dot(H,L),0.00001),2)));
			*/

			D =	((Alpha + 2.0)/(2.0*Pi)) * pow((HN),Alpha);
			F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,R),0.0)),5);
			G = (1.0/(pow(max(dot(H,R),0.00001),2)));


			//
			//The BRDF Equation
			//BRDF = (Kd/Pi) + (F * G * D) / 4.0;
			//BRDF = (F * G * D) / 4.0;

			//
			//FragColor.xyz = (Ia * Kd) + Ii * (LN)	*(BRDF * color.xyz);
			
			//
			// If in shadow, only apply ambient lighting
			inShadow = false;
			if (inShadow == true) 
			{
				//In Shadow
				//Ia is ambient Light
				FragColor.xyz = (Ia * Kd) + irradianceBRDF;  // Ambient lighting only + Irradiance
				

			} else
			{
				
				uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
				uv[1] = acos(R.z) / Pi;
				Ii = texture(SkyMapHDR, uv).xyz;
				//In lighting

				//Ii * (dot(R,N)) * (BRDF) +
				//FragColor.xyz = (Ia * Kd)  +  irradianceBRDF;
				//FragColor.xyz = Ii * (dot(R,N)) * (BRDF) + irradianceBRDF;
				//FragColor.xyz = Ii * (max(dot(R,N),0.00001)) * (BRDF) + irradianceBRDF;
				//FragColor.xyz = Ii * 0.03 + irradianceBRDF;
				FragColor.xyz = ((Ia * 0.5) * Kd) + Ii * 0.03 + irradianceBRDF;
				//Tone Mapping
				//FragColor.xyz = Kd;

				C = FragColor.xyz;
				C = pow((e*C) / (e * C + vec3(1,1,1)), vec3(1/2.2));
				FragColor.xyz = C;
			}


			FragColor.w = 1.0;
			
	}else if(objectId == skyId || objectId == seaId) {
		//
		//Only For Sky and sea Texture
		C = color.xyz;
		C = pow((e*C) / (e * C + vec3(1,1,1)), vec3(1/2.2));
		FragColor.xyz = C;
		FragColor.w = 1.0;
		
		//FragColor = color;
	}
	

}












