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
uniform sampler2D tex;//
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

uniform sampler2D WorldPosMap;
uniform sampler2D NMap;
uniform sampler2D KdMap;
uniform sampler2D KaMap;

uniform sampler2D BlurredShadowMap;
/*
uniform float z0;//z_near
uniform float z1;//z_far
*/
vec3 lightVec, eyeVec;
uniform vec3 lightPos;
uniform mat4 WorldInverse, ShadowMatrix;

uniform int AlgoNum;

int blurWidth = 2;

vec2 QuadraticRoots(float a, float b, float c)
{
	 //const float epsilon = 1e-6f;
	 float discriminant = b * b - 4 * a * c;
	

	 if (discriminant > 0)
	 {
		  float SquareRoot = sqrt(discriminant);
		  float root1 = (-b + SquareRoot) / (2.0f * a);
		  float root2 = (-b - SquareRoot) / (2.0f * a);

		  return vec2(min(root1, root2), max(root1, root2));
	 }
	 //else if (discriminant >= 0.0f && discriminant <= epsilon)
	 else //(discriminant <= epsilon)
	 {
		  //float root = -b / (2.0f * a);
		  //return vec2(root);
		  return vec2(0.0);
	 }
	 /*
	 else 
	 {
		  float real = -b / (2.0f * a);
		  float imag = sqrt(-discriminant) / (2.0f * a);
		  return vec2(real, imag);
	 }
	 */
}


vec3 CholeskyDecomposition(vec4 BPrime,float zf,float zf2){

	float m11 = 1.0f; 
	float m12 = BPrime.x;
	float m13 = BPrime.y;
	float m22 = BPrime.y;
	float m23 = BPrime.z;
	float m33 = BPrime.w;
	float z1 = 1.0f;
	float z2 = zf;
	float z3 = zf2;

	float a = sqrt(max(m11, 0.01));//sqrt(m11);
	float b = m12 / a;
	float c = m13 /a ;
	float d = sqrt(max(m22 - (b * b), 0.01));//sqrt(m22 - (b * b));
	float e = (m23 - (b * c) ) / d;
	float f = sqrt(max(m33 - (c * c) - (e * e), 0.01));//sqrt(m33 - (c * c) - (e * e));
	
	/*
	a = max(a, 0.0001);
	d = max(d, 0.0001);
	f = max(f, 0.0001);
	*/

	float c1_ = z1 / a;
	float c2_ = (z2 - (b * c1_)) / d;
	float c3_ = (z3 - (c * c1_) - (e * c2_)) / f;

	float c3 = c3_ / f;
	float c2 = (c2_ - (e * c3)) / d;
	float c1 = (c1_ - (b * c2) - (c * c3)) / a;

	return vec3(c1, c2, c3);

}


float MSM(vec4 BlurredValues, vec2 ShadowId, float PixelDepth){
	
	vec4 b = BlurredValues;
	float Alpha = 0.001;//1⋅10^−3

	vec4 B_Prime = ((1.0 - Alpha) * b) + (Alpha * vec4(0.5));

	float z_f = PixelDepth;
	float z_f2 = (z_f * z_f);

	vec3 C = CholeskyDecomposition(B_Prime, z_f, z_f2);

	vec2 Solve = QuadraticRoots(C.z, C.y, C.x);
	float z2 = Solve.x;
	float z3 = Solve.y;

	if(z2 == 0.0 && z3 == 0.0){

		return 0.0;
	
	}else if(z_f <= z2){// || abs(z_f - z2) <= 0.001f ) {//|| abs(z_f - z2) <= 0.001){
		
		return 0.0;

	}else if(z_f <= z3) {// || abs(z_f - z3) <= 0.001f){//|| abs(z_f - z3) <= 0.0001){
		
		float a = (z_f * z3) - B_Prime.x * (z_f + z3) + B_Prime.y;
		float b = (z3 - z2) * (z_f - z2);
		return (a/b); // 1.0f - (a/b);
	
	} else{

		float a1 = (z2 * z3) - B_Prime.x * (z2 + z3) + B_Prime.y;
		float b1 = (z_f - z2) * (z_f - z3);
		return 1.0f - (a1/b1);
	}

}



//
//Variance Map
float VarianceMap(vec4 BlurShadowMap, float t) {

	float Value, M1, M2, Sigma_Sqr, Mu;
	
	M1 = BlurShadowMap.x;//0.5
	M2 = BlurShadowMap.y;//0.25

	Mu = M1;//0.5

	Sigma_Sqr = max( (M2 - ( M1 * M1 )), 0.001);
	
	Value = Sigma_Sqr/(Sigma_Sqr + pow((t - Mu), 2));

	return (1 - Value);
}

float S;

void main()
{
	/*
	//
	uv = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
	FragColor = texture(shadowMap, uv);// / 50.0; // or similar
	//FragColor.xyz = vec3(texture(shadowMap, uv).x)/100.0; // or similar
	//FragColor = texture(BlurredShadowMap, uv); // or similar
	//FragColor.w = 1.0;
	return; // which disables all further code in the shader
	*/

	uv = gl_FragCoord.xy/vec2(750,750);

	vec3 eyePos = (WorldInverse * vec4(0,0,0,1)).xyz;
	eyeVec = eyePos - texture(WorldPosMap,uv).xyz;
	lightVec = lightPos - texture(WorldPosMap,uv).rgb;

	vec3 N = texture(NMap,uv).rgb;
    vec3 L = normalize(lightVec);
	vec3 V = normalize(eyeVec);
	
	vec3 H = normalize(L+V);
	float LN = max(dot(L,N),0.0);
	float HN = max(dot(H,N),0.0);

	vec3 Kd = texture(KdMap, uv).xyz;
	vec3 Ks = texture(KaMap, uv).xyz;

	vec3 Ii = Light;
	vec3 Ia = Ambient;
	float Alpha = texture(KaMap, uv).a;
		
	vec3 T = normalize(tanVec);
    vec3 B = normalize(cross(T,N));

	ShadowCoord = ShadowMatrix *  texture(WorldPosMap, uv);

	shadowIndex = ShadowCoord.xy/ShadowCoord.w;

	vec2 SAT_step = 1.0f/textureSize(BlurredShadowMap, 0);
	float x_min = shadowIndex.x - (blurWidth * SAT_step.x);
	float x_max = shadowIndex.x + (blurWidth * SAT_step.x);
	float y_min = shadowIndex.y - (blurWidth * SAT_step.y);
	float y_max = shadowIndex.y + (blurWidth * SAT_step.y);
	/*
	 pixelDepth = ShadowCoord.w - 0.05;
	 FragColor.xyz = vec3(pixelDepth/130.0);
	 //FragColor.xyz = vec3(1.0,0.0,0.0);
	 FragColor.w = 1.0;
	 return;
	 */

	if (shadowIndex.x >= 0.0 && shadowIndex.x <= 1.0 &&
        shadowIndex.y >= 0.0 && shadowIndex.y <= 1.0 &&
        ShadowCoord.w > 0.0)
    {
        // Sample the shadow map (depth value from light's perspective)
        lightDepth = texture(shadowMap, shadowIndex).x;
        pixelDepth = (ShadowCoord.w - 0.01 - 0.1f) / (100.0f - 0.1f);// + 1.6;

		/*
		FragColor.xyz = vec3(pixelDepth/130.0);
		FragColor.w = 1.0;
		return;
		*/

		/*
		uv = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
		//FragColor.xyz = vec3(texture(shadowMap, uv).w/100.0); // or similar
		//FragColor.xyz = vec3(lightDepth/10.0); // or similar
		FragColor.xy = vec2(shadowIndex); // or similar
		FragColor.w = 1.0;
		return;
		*/

		
		if(AlgoNum == 0){
			
			// Determine if the fragment is in shadow (if pixelDepth > lightDepth)
			inShadow =  (pixelDepth > lightDepth);
		
		}else if(AlgoNum == 1){

			vec4 A_Value = texture(BlurredShadowMap, vec2(x_min, y_min));
			vec4 B_Value = texture(BlurredShadowMap, vec2(x_max, y_min));
			vec4 C_Value = texture(BlurredShadowMap, vec2(x_min, y_max));
			vec4 D_Value = texture(BlurredShadowMap, vec2(x_max, y_max));

			vec4 Sum = (D_Value - B_Value - C_Value + A_Value) / (4.0f * blurWidth * blurWidth);
			S = VarianceMap(Sum, pixelDepth);
		
		}else if(AlgoNum == 2){
		
			vec4 A_Value = texture(BlurredShadowMap, vec2(x_min, y_min));
			vec4 B_Value = texture(BlurredShadowMap, vec2(x_max, y_min));
			vec4 C_Value = texture(BlurredShadowMap, vec2(x_min, y_max));
			vec4 D_Value = texture(BlurredShadowMap, vec2(x_max, y_max));

			vec4 Sum = (D_Value - B_Value - C_Value + A_Value) / (4.0f * blurWidth * blurWidth);
			S = MSM(Sum, shadowIndex, pixelDepth);
		}

		//FragColor = vec4(S,S,S,1.0);
		//return;
	
    }
    else
    {
		/*
		uv = gl_FragCoord.xy/vec2(750,750); // (or whatever screen size)//750,750 given in test pdf
		FragColor.xyz = vec3(0.0,0.0,1.0); // or similar
		FragColor.w = 1.0;
		return; // which
		*/
        inShadow = false;  // Default to being in shadow if out of bounds
    }

	
	H = normalize(L+V);
	LN = max(dot(L,N),0.0);
	HN = max(dot(H,N),0.0);

	//
	//Normal Calculations are done above 
	//reflections are to be done here 
	if(objectId == seaId){

	 	vec3 R = -1.0 * (2.0 * dot(N , V) * (N - V));
		uv[0] = -atan(R.y, R.x) / (2.0 * Pi);
		uv[1] = acos(R.z) / Pi;
		color = texture(tex,uv);
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
		if(AlgoNum == 0){
			
			// If in shadow, only apply ambient lighting
			if (inShadow == true) 
			{
				//In Shadow
				FragColor.xyz = (Ia * Kd);  // Ambient lighting only

			} else if (inShadow == false) 
			{
				//In lighting
				FragColor.xyz = (Ia * Kd) +  Ii * (LN)	* (BRDF);
				//FragColor.xyz = vec3(0.0, 1.0, 0.0);
			}

		}else if(AlgoNum == 1){

			//FragColor.xyz = (Ia * Kd) + Ii * (LN)	* (BRDF);
			FragColor.xyz = (Ia * Kd) + (1 - S) * Ii * (LN)	* (BRDF);
		
		}else if(AlgoNum == 2){

			FragColor.xyz = (Ia * Kd) + (1 - S) * Ii * (LN)	* (BRDF);

		}

			FragColor.w = 1.0;

	}else if(objectId == skyId || objectId == seaId) {
		//
		//Only For Sky and sea Texture
		FragColor = color;
	}


	

}




