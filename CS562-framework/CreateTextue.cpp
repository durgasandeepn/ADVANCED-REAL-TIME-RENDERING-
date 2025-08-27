#include <fstream>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include "glu.h"
#include"CreateTextue.h"

CreateTexture::CreateTexture() {

}


void  CreateTexture::CreatingTexture(int Width, int Height, unsigned int* TextureId ) {

        glGenTextures(1, TextureId);
        glBindTexture(GL_TEXTURE_2D, *TextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, (int)GL_RGBA32F, Width, Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
}


void CreateTexture::BindTexture(const int unit, const int programId, const std::string& name,GLint TextureID)
{//

    glActiveTexture((gl::GLenum)((int)GL_TEXTURE0 + unit));
    //CHECKERROR;
    glBindTexture(GL_TEXTURE_2D, TextureID);
    //CHECKERROR;
    int loc = glGetUniformLocation(programId, name.c_str());
    glUniform1i(loc, unit);

}

 



