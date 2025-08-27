#ifndef _HDR_TEXTURE_
#define _HDR_TEXTURE_

#include<string>
#include "Texture.h"


class HDRTexture : public Texture
{
public:
    float* data;
    int width, height, channels;
    HDRTexture(const std::string& filename);
    HDRTexture( );
    void HDR_ReadAsPixel(const std::string& filename);
};


#endif







