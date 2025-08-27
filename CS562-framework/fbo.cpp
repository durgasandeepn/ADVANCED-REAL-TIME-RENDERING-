///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include "fbo.h"

#include "glu.h"

#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line Fbo.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }


FBO::FBO(const int w, const int h) {
    width = w;
    height = h;
}


void FBO::CreateFBO(const int w, const int h)
{
    width = w;
    height = h;

    glGenFramebuffersEXT(1, &fboID);//Gen to generate 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

    // Create a render buffer, and attach it to FBO's depth attachment
    unsigned int depthBuffer;
    glGenRenderbuffersEXT(1, &depthBuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                             width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT, depthBuffer);

    // Create a texture and attach FBO's color 0 attachment.  The
    // GL_RGBA32F and GL_RGBA constants set this texture to be 32 bit
    // floats for each of the 4 components.  Many other choices are
    // possible.
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                              GL_TEXTURE_2D, textureID, 0);
    
    // Check for completeness/correctness
    int status = (int)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != int(GL_FRAMEBUFFER_COMPLETE_EXT))
        printf("FBO Error: %d\n", status);

    //glObjectLabel(GL_FRAMEBUFFER, fboID, -1, "DEBUG FBO SHADOW");
    //glObjectLabel(GL_TEXTURE, textureID, -1, "DEBUG FBO TEXTURE");

    //
    // Unbind the fbo until it's ready to be used
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    CHECKERROR;
}



void FBO::CreateFBO_Multi(const int w, const int h)
{
    width = w;
    height = h;

    glGenFramebuffersEXT(1, &fboID);//Gen to generate 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

    // Create a render buffer, and attach it to FBO's depth attachment
    unsigned int depthBuffer;
    glGenRenderbuffersEXT(1, &depthBuffer);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
        width, height);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
        GL_RENDERBUFFER_EXT, depthBuffer);

    // Create a texture and attach FBO's color 0 attachment.  The
    // GL_RGBA32F and GL_RGBA constants set this texture to be 32 bit
    // floats for each of the 4 components.  Many other choices are
    // possible.
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_2D, textureID, 0);
    //
    glGenTextures(1, &textureID2);
    glBindTexture(GL_TEXTURE_2D, textureID2);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
        GL_TEXTURE_2D, textureID2, 0);
    //
    glGenTextures(1, &textureID3);
    glBindTexture(GL_TEXTURE_2D, textureID3);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT,
        GL_TEXTURE_2D, textureID3, 0);
    //
    glGenTextures(1, &textureID4);
    glBindTexture(GL_TEXTURE_2D, textureID4);
    glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT3_EXT,
        GL_TEXTURE_2D, textureID4, 0);
  
    
    GLenum bufs[4] = { GL_COLOR_ATTACHMENT0_EXT , GL_COLOR_ATTACHMENT1_EXT ,
        GL_COLOR_ATTACHMENT2_EXT , GL_COLOR_ATTACHMENT3_EXT };
    glDrawBuffers(4, bufs);
    

    //
    // Check for completeness/correctness
    int status = (int)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    if (status != int(GL_FRAMEBUFFER_COMPLETE_EXT))
        printf("FBO Error: %d\n", status);

    //
    // Unbind the fbo until it's ready to be used
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    CHECKERROR;
}


unsigned int FBO::getFBOID( ) {
    return fboID;
}

void FBO::BindFBO() { 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID); 
    //CHECKERROR;
}

void FBO::UnbindFBO() { 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 
    //CHECKERROR;
}

void FBO::BindTexture(const int unit, const int programId, const std::string& name)
{//

    glActiveTexture((gl::GLenum)((int)GL_TEXTURE0 + unit));
    //CHECKERROR;
    glBindTexture(GL_TEXTURE_2D, textureID);
    //CHECKERROR;
    int loc = glGetUniformLocation(programId, name.c_str());
    glUniform1i(loc, unit);
}


void FBO::BindImageTexture_CS(const int unit,const GLenum access, const GLenum format,int ShaderId, char* name)
{//

   glBindImageTexture(unit, textureID, 0, GL_FALSE, 0, access, format);
   int loc = glGetUniformLocation(ShaderId, name);
   glUniform1i(loc, unit);

}



void FBO::BindTexture4(const int unit, const int programId, const std::string& name)
{//

    glActiveTexture((gl::GLenum)((int)GL_TEXTURE0 + unit));
    //CHECKERROR;
    //glBindTexture(GL_TEXTURE_2D, textureID);
    
    if (unit == 3) {
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    else if (unit == 4) {
        glBindTexture(GL_TEXTURE_2D, textureID2);
    }
    else if (unit == 5) {
        glBindTexture(GL_TEXTURE_2D, textureID3);
    }
    else if (unit == 6) {
        glBindTexture(GL_TEXTURE_2D, textureID4);
    }
    
    //CHECKERROR;
    int loc = glGetUniformLocation(programId, name.c_str());
    glUniform1i(loc, unit);
}


void FBO::UnbindTexture(const int unit)
{  
    glActiveTexture((gl::GLenum)((int)GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, 0);
    //CHECKERROR;

}




/*
void FBO::CreateSSBO(size_t lightsSize, const PointLightData& light) {

    // Generate and bind SSBO
    glGenBuffers(1, &ssboID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);

    //size_t lightsSize = pointLights.size()
    indicesSize = textureIDs.size() * sizeof(int);
    
    // Allocate memory for the SSBO (e.g., to store 4 color attachment data)
    //glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBufferData(GL_SHADER_STORAGE_BUFFER, lightsSize * indicesSize, nullptr, GL_DYNAMIC_DRAW);

    
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, lightsSize, light.data());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, lightsSize, indicesSize, textureIDs.data());

    // Bind SSBO to binding point (binding point 0 in this example)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboID);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
    CHECKERROR;
}


void FBO::PopulateSSBOFromTextures() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);

    // Create a vector to store texture data
    std::vector<glm::vec4> textureData(4);

    // Bind each texture and read its data
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, textureIDs[i]); // Use your texture ID array
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &textureData[i]);
    }

    // Upload data to SSBO
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(glm::vec4), textureData.data());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind SSBO
    CHECKERROR;
}


void FBO::LoadTextureIDs( ) {
    
    textureIDs.push_back(3);
    textureIDs.push_back(4);
    textureIDs.push_back(5);
    textureIDs.push_back(6);

}

*/


