/////////////////////////////////////////////////////////////////////////
// Vertex shader for lighting
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 430

//uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMatrix;

in vec4 vertex;
in vec2 vertexTexture;
out vec3 tanVec;
out vec2 texCoord;//

/*
in vec3 vertexNormal;//
in vec3 vertexTangent;

out vec3 normalVec, lightVec, eyeVec;
uniform vec3 lightPos;

out vec4 ShadowCoord;
*/

void main()
{      
	/*
	tanVec =  mat3(ModelTr)*vertexTangent; 

    gl_Position = WorldProj * WorldView * ModelTr * vertex;
	
	ShadowCoord = ShadowMatrix * ModelTr * vertex;//
    
    vec3 worldPos = (ModelTr*vertex).xyz;

    normalVec = vertexNormal*mat3(NormalTr); 
    lightVec = lightPos - worldPos;
	
	vec3 eyePos =(WorldInverse*vec4(0,0,0,1)).xyz;
	lightVec=lightPos-worldPos;
	eyeVec =eyePos-worldPos;
    
	texCoord = vertexTexture;// 
	*/

	texCoord = vertexTexture;//06-04-2025

	gl_Position = vertex;

}

