///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

class ShaderProgram
{
public:
    int programId;
    GLuint BlurSMTextureId, FinalSMTextureId;


    ShaderProgram();
    void AddShader(const char* fileName, const GLenum type);
    void LinkProgram();
    void UseShader();
    void UnuseShader();


    ShaderProgram(int i);//constructor
    void LinkProgram_Compute(int PrgId);
    void UseShader_CS(int i);

    //void CreateTexture(int Width, int Height, unsigned int TextureId);
    void BindImageTexture(int unit, unsigned int Textureid, const GLenum access, int ShaderId, char* name);
    void BindTexture(const int unit, const int programId, const std::string& name);
    void DispatchComputerShader(int Width, int Height, int group_x, int group_y);
};
