///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

#include <fstream>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include "glu.h"
#include "shader.h"




// Reads a specified file into a string and returns the string.  The
// file is examined first to determine the needed string size.
char* ReadFile(const char* name)
{
    std::ifstream f;
    f.open(name, std::ios_base::binary); // Open
    if (f.fail()) { printf("\nERROR! Can't read/find shader file: %s\n\n", name); exit(-1); }
    f.seekg(0, std::ios_base::end);      // Position at end
    int length = f.tellg();              //   to get the length

    char* content = new char [length+1]; // Create buffer of needed length
    f.seekg (0, std::ios_base::beg);     // Position at beginning
    f.read (content, length);            //   to read complete file
    f.close();                           // Close

    content[length] = char(0);           // Finish with a NULL
    return content;
}

// Creates an empty shader program.
ShaderProgram::ShaderProgram()
{ 
    programId = glCreateProgram();
}


// Use a shader program
void ShaderProgram::UseShader()
{
    glUseProgram(programId);
}

// Done using a shader program
void ShaderProgram::UnuseShader()
{
    glUseProgram(0);
}

// Read, send to OpenGL, and compile a single file into a shader
// program.  In case of an error, retrieve and print the error log
// string.
void ShaderProgram::AddShader(const char* fileName, GLenum type)
{
    // Read the source from the named file
    char* src = ReadFile(fileName);
    const char* psrc[1] = {src};

    // Create a shader and attach, hand it the source, and compile it.
    int shader = glCreateShader(type);
    glShaderSource(shader, 1, psrc, NULL);
    glCompileShader(shader);
    glAttachShader(programId, shader);
    delete src;

    // Get the compilation status
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    
    // If compilation status is not OK, get and print the log message.
    if (status != 1) {
        int length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* buffer = new char[length];
        glGetShaderInfoLog(shader, length, NULL, buffer);
        printf("Compile log for %s:\n%s\n", fileName, buffer);
        delete buffer;
    }
}

// Link a shader program after all the shader files have been added
// with the AddShader method.  In case of an error, retrieve and print
// the error log string.
void ShaderProgram::LinkProgram()
{
    // Link program and check the status
    glLinkProgram(programId);
    int status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    
    // If link failed, get and print log
    if (status != 1) {
        int length;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);
        char* buffer = new char[length];
        glGetProgramInfoLog(programId, length, NULL, buffer);
        printf("Link log:\n%s\n", buffer);
        delete buffer;
    }
}


//For Compute Shader
void ShaderProgram::UseShader_CS(int i){
 
    glUseProgram(programId);

}


/*
void ShaderProgram::CreateTexture(int Width, int Height) {

    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, Width, Height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
*/


// the error log string.
void ShaderProgram::LinkProgram_Compute(int PrgId)
{
    // Link program and check the status
    glLinkProgram(PrgId);
    int status;
    glGetProgramiv(PrgId, GL_LINK_STATUS, &status);

    // If link failed, get and print log
    if (status != 1) {
        int length;
        glGetProgramiv(PrgId, GL_INFO_LOG_LENGTH, &length);
        char* buffer = new char[length];
        glGetProgramInfoLog(PrgId, length, NULL, buffer);
        printf("Link log:\n%s\n", buffer);
        delete[] buffer;
    }
}



void ShaderProgram::BindImageTexture(int unit, unsigned int Textureid, const GLenum access, int ShaderId, char* name) {
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, Textureid);  
    //glBindImageTexture(id, Textureid, 0, GL_FALSE, 0, access, GL_RGBA32F);
    glBindImageTexture(unit, Textureid, 0, GL_FALSE, 0, access, GL_RGBA32F);
    int loc = glGetUniformLocation(ShaderId, name);
    glUniform1i(loc, unit);
}



void ShaderProgram::BindTexture(const int unit, const int programId, const std::string& name)
{//
    /*
    glActiveTexture((gl::GLenum)((int)GL_TEXTURE0 + unit));
    //CHECKERROR;
    glBindTexture(GL_TEXTURE_2D, FinalSMTextureId);
    //CHECKERROR;
    int loc = glGetUniformLocation(programId, name.c_str());
    glUniform1i(loc, unit);
    */
}


void ShaderProgram::DispatchComputerShader(int Width, int Height, int group_x, int group_y) {

    glDispatchCompute(Width / group_x, Height / group_y, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}

/*
void ShaderProgram::BindImageTexture(int id, unsigned int Textureid) {
    glBindImageTexture(1, Textureid, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}
*/

/*
void ShaderProgram::BindtextureToSample( ,unsigned int textureId) {
    // Activate texture unit for shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurredShadowMapTexture);

    glUniform1i(glGetUniformLocation(lightingShader, "BlurredShadowMap"), 1);

}

*/