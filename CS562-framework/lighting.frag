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
in vec2 texCoord;//
uniform sampler2D tex;//For wall tiles
//uniform sampler2D normalMap;

//Shadow
//uniform sampler2DShadow shadowMap;
uniform sampler2D shadowMap;
vec4 ShadowCoord;
vec2 shadowIndex;
float lightDepth;
float pixelDepth;
bool inShadow;
//
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
vec2 uv1;//
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);//

bool TexturePresent = false;
bool NormalMapPresent = false;

float checker;
uniform vec3 color1; // Color of the first checker
uniform vec3 color2; // Color of the second checker
uniform float scale; // Scale of the checkerboard
vec3 delta;

//
//IBL START

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

//
//
//
//Added these values ---> 30-04-2025
const uint Hammersley_Num = 20;

const vec2 HammersleySeq[20] = vec2[20](
    vec2(0.0, 0.01),
    vec2(0.5, 0.03),
    vec2(0.25, 0.05),
    vec2(0.75, 0.07),
    vec2(0.125, 0.09),
    vec2(0.625, 0.11),
    vec2(0.375, 0.13),
    vec2(0.875, 0.15),
    vec2(0.0625, 0.17),
    vec2(0.5625, 0.19),
    vec2(0.3125, 0.21),
    vec2(0.8125, 0.23),
    vec2(0.1875, 0.25),
    vec2(0.6875, 0.27),
    vec2(0.4375, 0.29),
    vec2(0.9375, 0.31),
    vec2(0.03125, 0.33),
    vec2(0.53125, 0.35),
    vec2(0.28125, 0.37),
    vec2(0.78125, 0.39));



float G1(vec3 v, vec3 H, float Alpha, vec3 N){

	float g1;
	float Tan = sqrt( (1.0f - pow( dot(v,N), 2) ) ) / dot(v,N);
	float a = sqrt( Alpha /(2.0f + 1.0f)) / Tan; 
	//float a = 0;

	if(a <  1.6f)
	{

		g1 = ((3.535f * a) + 2.181f * (a * a)) / ( 1.0f + (2.276f * a) + (2.577f * a * a) );

	}else {
	
		g1 = 1.0f;
	}


	return g1;
}



//IBL END
//



//
uniform sampler2D BlurredShadowMap;
uniform sampler2D AOFinalBlurMap;

//
uniform sampler2D WorldPosMap;
uniform sampler2D NMap;
uniform sampler2D KdMap;
uniform sampler2D KaMap;

uniform mat4 WorldInverse,ShadowMatrix;//WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMatrix;

uniform vec3 lightPos1;//LIGHT POSITION
vec3 eyePos;
vec3 normalVec, lightVec, eyeVec;
//in vec2 texCoord;//06-04-2025 - needed
	
uniform float z0;//z_near
uniform float z1;//z_far

float S;

uniform int AlgoNum;



void main( )
{	

	ivec2 TextureSize = textureSize(SkyMapHDR, 0);
	float HEIGHT = TextureSize.y, WIDTH = TextureSize.x;

	//
	//DEBUGGING 

	/*
	//ShadowMap Reading Correctly ... working
	vec2 uv = gl_FragCoord.xy/vec2(750,750); // Or use textureSize
    FragColor = texture(AOFinalBlurMap, uv);
	return;
	*/

	uv = gl_FragCoord.xy/vec2(1024,1024);
	ShadowCoord = ShadowMatrix *  texture(WorldPosMap, uv);
	shadowIndex = ShadowCoord.xy/ShadowCoord.w;
	

	eyePos = (WorldInverse * vec4(0,0,0,1)).xyz;
	eyeVec = eyePos - texture(WorldPosMap, uv).xyz;
	lightVec = lightPos1 - texture(WorldPosMap, uv).rgb;//light


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
	float Alpha = texture(KaMap, uv).a * 10;
	
	vec3 T = normalize(tanVec);
    vec3 B = normalize(cross(T,N));

	shadowIndex = ShadowCoord.xy/ShadowCoord.w;
	uv = gl_FragCoord.xy/vec2(1024,1024);
	

	if (shadowIndex.x >= 0.0 && shadowIndex.x <= 1.0 &&
        shadowIndex.y >= 0.0 && shadowIndex.y <= 1.0 &&
        ShadowCoord.w > 0.0)
    {
        // Sample the shadow map (depth value from light's perspective)
		lightDepth = texture2D(shadowMap, shadowIndex).x;
        //pixelDepth = (ShadowCoord.w - z0)/(z1 - z0);//zf 
        pixelDepth = ShadowCoord.w - 0.0;//zf 
		//float pd =  ShadowCoord.w - 0.05;

		inShadow =  (pixelDepth > lightDepth);
		
	}
    else
    {
		inShadow = false;  // Default to being in shadow if out of bounds

//		if(AlgoNum == 0){
//			
//		}
	
	}

	if(objectId == teapotId)
	{

	}

	if(objectId == seaId){

	 	R = -1.0 * (2.0 * dot(N , V) * (N - V));
		R  = normalize(R);
		uv1[0] = -atan(R.y, R.x) / (2.0 * Pi);
		uv1[1] = acos(R.z) / Pi;
		color = texture(tex,uv1);

	}
	
	//
	//THE LIGHTING
	if(objectId != skyId && objectId != seaId){

			uv1[0] = -atan(-N.y, -N.x) / (2 * Pi);
			uv1[1] = acos(N.z) / Pi;

			R = -1.0 * (2.0 * dot(N , V) * N - V);
			
			if(objectId != teapotId){
				e = 2.5;
				Kd = pow(Kd , vec3(2.2)); 
				//Kd = pow((e*Kd) / (e * Kd + vec3(1,1,1)), vec3(2.2));
			}
			
			irradianceBRDF = texture(IrradianceMap, uv1).xyz * Kd/Pi * 10 * texture(AOFinalBlurMap, uv).x;
			
			//
			//PROJECT 3
			//Specular Calculations
			vec3 sum = vec3(0.0f);
			vec3 R_ = 2.0 * dot(N,V) * N - V;//Check this
			//vec3 R_ = reflect(V,N);// 2.0 * dot(N,V) * N - V;//Check this
			vec3 a = normalize(vec3(-R_.y, R_.x, 0.0));
			vec3 b = normalize(cross(R_, a));
			
			for(int n = 0; n < 20; n++){
			 
				//Used the Phong BRDF for the D(H) term
				float Theta = acos( pow( HammersleySeq[n].y, ( 1.0 / (Alpha + 1.0) ) ) );
			
				vec3 d; 
				d.x = cos( (2.0 * Pi) * ((0.5f) - HammersleySeq[n].x)) * sin(Pi * (Theta/Pi));
				d.y = sin( (2.0 * Pi) * ((0.5f) - HammersleySeq[n].x)) * sin(Pi * (Theta/Pi)); 
				d.z = cos(Pi * (Theta/Pi)); 

				vec3 Omega_K = normalize((d.x * a) + (d.y * b) + (d.z * R_));//Omega K
				vec3 H = normalize(Omega_K + V);


				D = ((Alpha + 2) / (2 * Pi)) * pow( dot(N,H), Alpha);

				//UV of Omega
				vec2 UVofOmega;
				UVofOmega.x = (0.5f) - (atan(Omega_K.y, Omega_K.x)/(2.0f * Pi));
				UVofOmega.y = acos(Omega_K.z) / Pi;
				//Level
				float level = (0.5f) * log2( float(WIDTH * HEIGHT) / float(n) ) - 0.5f * log2(D/4.0) ;
				
				//
				//SkyMapHDR
				G = G1(Omega_K, H, Alpha, N) * G1(V, H, Alpha, N);

				//G = (1.0/(pow(max(dot(H,Omega_K),0),2)));
				F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,Omega_K),0.0)),5);
				//Specular = D(H) * F(L,H) /(4 * LH^2)
				vec3 Spec = (G * F) / (4.0f * dot(Omega_K, N) * dot(V, N));

				sum += textureLod(SkyMapHDR, UVofOmega, level).rgb * max( dot(N, Omega_K),0) * Spec;//( (G * F) / (4 * dot(Omega_K, N) * dot(V,N) ) );
				//sum += textureLod(SkyMapHDR, UVofOmega, level).rgb;//( (G * F) / (4 * dot(Omega_K, N) * dot(V,N) ) );
				
			}
			vec3 SpecIrradiance = max((1.0f/20.0f) * sum, 0) * texture(AOFinalBlurMap, uv).x;

			//
			// If in shadow, only apply ambient lighting

			/*
			inShadow = false;
			if (inShadow == true) 
			{
				//In Shadow
				//Ia is ambient Light
				//FragColor.xyz = (Ia * Kd * value) + irradianceBRDF;  // Ambient lighting only + Irradiance
				//FragColor.xyz = Kd / Pi * irradianceBRDF + value;
			} else
			{
			}
			*/

				uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
				uv[1] = acos(R.z) / Pi;
				Ii = texture(SkyMapHDR, uv).xyz;
				
				//In lighting
				FragColor.xyz = irradianceBRDF + SpecIrradiance;
				
				//Tone Mapping
				//FragColor.xyz = Kd;
				C = FragColor.xyz;
				C = pow((e*C) / (e * C + vec3(1,1,1)), vec3(1/2.2));
				//C = (e*C) / (e * C + vec3(1,1,1));
				FragColor.xyz = C;

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
	

//	// ******
//	//this chuck changes below
//
//	H = normalize(L+V);
//	LN = max(dot(L,N),0.0);
//	HN = max(dot(H,N),0.0);
//
//	
//	//
//	//THE LIGHTING
//	//
//	//Phong Lighting Calculations
//	//FragColor.xyz = vec3(0.5,0.5,0.5)*Kd + Kd*max(dot(L,N),0.0);
//	//
//	if(objectId != skyId && objectId != seaId){
//	
//		D =	((Alpha + 2.0)/(2.0*Pi)) * pow((HN),Alpha);
//		F = Ks + ((1,1,1) - Ks) * pow((1 - max(dot(H,L),0.0)),5);
//		G = (1.0/(pow(max(dot(H,L),0.00001),2)));
//	
//		BRDF = (Kd/Pi) + (F * G * D) / 4.0;
//
//		//FragColor.xyz = (Ia * Kd) + Ii*(LN)	*(BRDF * color.xyz);
//		
//			
//
//		if(AlgoNum == 0){
//			/*
//				// If in shadow, only apply ambient lighting
//				if (inShadow == true) //Ambient Lighting
//				{
//					//In Shadow
//					//FragColor.xyz = vec3(1.0f,0.0f,0.0f);  // Ambient lighting only
//					FragColor.xyz = (Ia * Kd);// *  texture(AOFinalBlurMap, uv).xyz;  // Ambient lighting only
//
//				} else if (inShadow == false) 
//				{
//					//In lighting
//					//FragColor.xyz = (Ia * Kd) + S *(Ii*(LN)	* (BRDF));
//					FragColor.xyz = (Ia * Kd) + Ii * (LN) * (BRDF);
//				}
//			*/
//		}else if(AlgoNum == 1){
//			
//			//FragColor.xyz = ( (Ia * Kd) * texture(AOFinalBlurMap, uv).xyz ) + (1 - S) * (Ii * (LN)	* (BRDF));
//			//FragColor.xyz = ( (Ia * Kd) * texture(AOFinalBlurMap, uv).xyz ) + (Ii * (LN)	* (BRDF));
//			//FragColor.xyz = (Ia * Kd) + Ii * (LN) * (BRDF);
//
//		}
//
//
//		//FragColor.xyz = (Ia * Kd) + S *(Ii*(LN)	* (BRDF));
//		FragColor.w = 1.0;
//		
//	}else if(objectId == skyId || objectId == seaId) {
//		//
//		//Only For Sky and sea Texture
//		FragColor = color;
//	}
//
//
//	//this chuck changes above
//	// ******

}
