///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////
#include <glm/glm.hpp>

class FBO {
public:

    unsigned int fboID;
    unsigned int textureID, textureID2, textureID3, textureID4;
    std::vector<GLuint> textureIDs;
    int width, height;  // Size of the texture.

    //SSBO
    unsigned int ssboID;
    size_t indicesSize;

    FBO(const int w, const int h);
    void CreateFBO(const int w, const int h);
    void CreateFBO_Multi(const int w, const int h);
    
    // Bind this FBO to receive the output of the graphics pipeline.
    void BindFBO();
    
    // Unbind this FBO from the graphics pipeline;  graphics goes to screen by default.
    void UnbindFBO();

    // Bind this FBO's texture to a texture unit.
    void BindTexture(const int unit, const int programId, const std::string& name);
    void BindTexture4(const int unit, const int programId, const std::string& name);

    // Unbind this FBO's texture from a texture unit.
    void UnbindTexture(const int unit);
    void BindImageTexture_CS(const int unit, const GLenum access, const GLenum format, int ShaderId, char* name);

    void BindImageTexture_AO(const int unit, const GLenum access, const GLenum format, int ShaderId, char* name);

    unsigned int getFBOID( );

    /*
    void LoadTextureIDs();
    void CreateSSBO(size_t lightsSize);
    void PopulateSSBOFromTextures();//cannot say need to use it or not 
    */
};



