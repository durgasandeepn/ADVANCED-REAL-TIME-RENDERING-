
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include "glu.h"
#include <iostream>
#include <vector>

using namespace std;

#pragma once
class Project2
{
public:
	GLuint bufferId;
	int bindpoint, bindpointAO;
	int Size = 101;
	// Kernel size variables
	int blurWidth = 2; // Initial half-width of the kernel
	const int maxBlurWidth = 100; // Max half-width
	const int minBlurWidth = 1;   // Min half-width
	std::vector<float> weights; // Kernel weights (fixed size of 101)
	
	bool Flag;

	Project2();
	Project2(int BpValue);
	void CreateUniformBuffer( );
	void UpdateWeights( );

	void IncrementKernal( );
	void DecrementKernal( );
	int  KernalValue();

};
