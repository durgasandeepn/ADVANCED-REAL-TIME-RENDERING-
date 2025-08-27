/////////////////////////////////////////////////////////////////////////
// Vertex shader for lighting
//
// Copyright 2013 DigiPen Institute of Technology
////////////////////////////////////////////////////////////////////////
#version 430

//uniform mat4 WorldView, WorldInverse, WorldProj, ModelTr, NormalTr, ShadowMatrix;
uniform mat4 WorldView, WorldProj, WorldInverse, ModelTr, NormalTr;

in vec4 vertex;
in vec3 vertexNormal;
//in vec2 vertexTexture;//
in vec3 vertexTangent;

//out vec3 tanVec;

//out vec3 normalVec, eyeVec;// lightVec;
//out vec2 texCoord;//
uniform vec3 lightPos;

//out vec3 worldPos;

out vec3 eyeVec;


//
void main()
{      
	//tanVec =  mat3(ModelTr)*vertexTangent; 
	
    gl_Position = WorldProj * WorldView * vertex;//this is given as gl_Position=vertex;
	
    vec3 worldPos = (ModelTr*vertex).xyz;

    //normalVec = vertexNormal*mat3(NormalTr);
    //lightVec = lightPos - worldPos;
	
	vec3 eyePos = (WorldInverse*vec4(0,0,0,1)).xyz;

	eyeVec = eyePos - worldPos;
	
	//texCoord = vertexTexture;

	//gl_Position = vertex;

}

