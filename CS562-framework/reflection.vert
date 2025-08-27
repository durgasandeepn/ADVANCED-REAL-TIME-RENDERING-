/////////////////////////////////////////////////////////////////////////
// Reflection Vertex shader
////////////////////////////////////////////////////////////////////////
#version 330

uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMatrix;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexture;//
in vec3 vertexTangent;

out vec3 tanVec;

out vec3 normalVec, lightVec, eyeVec;
out vec2 texCoord;//
uniform vec3 lightPos;

out vec4 ShadowCoord;

//Reflection
vec3 P;
vec3 Eye;
vec3 R;
uniform float SIGN;
uniform bool FLAG;
float Length;

void main()
{      
	Eye = vec3(0, 0, 1.5);
//	eyeVec = Eye;
	tanVec =  mat3(ModelTr) * vertexTangent; 
	
	//
	//Reflection Maps
	P = (ModelTr * vertex).xyz;
	Length = length(P - Eye);
	R = normalize(P - Eye);
	float c = SIGN * R.z;
	//float c = R.z;
	
    //old code not needed here -- change it
    //gl_Position = WorldProj * WorldView * ModelTr * vertex;
	//
	gl_Position = vec4(R.x / (1 + c), R.y / (1 + c), ((c * Length) / 1000.0) - 1.0, 1.0);

	/*
	if(FLAG == true){
		gl_Position = vec4(R.x / (1 + c), R.y / (1 + c), ((c * Length) / 1000.0) - 1.0, 1.0);
	}else{
		gl_Position = vec4(R.x / (1 - c), R.y / (1 - c), ((c * Length) / 1000.0) - 1.0, 1.0);
	}
	*/

	//Reflection Maps
	//End
	//
	ShadowCoord = ShadowMatrix * ModelTr * vertex;//
    
    vec3 worldPos = (ModelTr * vertex).xyz;

    normalVec = vertexNormal * mat3(NormalTr); 
    lightVec = lightPos - worldPos;
	
	vec3 eyePos = (WorldInverse * vec4(0,0,0,1)).xyz;
	eyeVec = Eye - worldPos;
    
	texCoord = vertexTexture;

	

}

