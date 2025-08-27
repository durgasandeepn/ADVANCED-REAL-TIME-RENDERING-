////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

#include "shapes.h"
#include "object.h"
#include "texture.h"
#include"HDRTexture.h"
#include "fbo.h"

enum ObjectIds {
    nullId	= 0,
    skyId	= 1,
    seaId	= 2,
    groundId	= 3,
    roomId	= 4,
    boxId	= 5,
    frameId	= 6,
    lPicId	= 7,
    rPicId	= 8,
    teapotId	= 9,
    spheresId	= 10,
    floorId     = 11
};

class Shader;


class Scene
{
public:
    GLFWwindow* window;

    // @@ Declare interactive viewing variables here. (spin, tilt, ry, front back, ...)
    float spin;
    float tilt;
    float tx;
    float ty;
    float zoom;
    float rx;
    float ry;
    float front;
    float back;
    float Rx, Ry;

    glm::vec3 eye = glm::vec3(0, -20, 0);
    float Speed;
    bool w_down;
    bool a_down;
    bool s_down;
    bool d_down;
    bool transformation_mode;
    double time_since_last_refresh;
    float step;
    double CurrentTime;
    double PreviousTime;

    int ShadowMap_Width, ShadowMap_Height;
    int ReflectionMap_Width, ReflectionMap_Height;
    //
    float e;
    //
    bool DepthFlag = false;
    /*
    enum ShadowTest { DepthTestTest, ShadowIndexTest, PixelDepthTest, LightDepthTest};
    ShadowTest ShadowTesting = DepthTestTest;
    */
    //
    
    // 
    // Light parameters
    float lightSpin, lightTilt, lightDist;
    glm::vec3 lightPos;
    glm::vec3 lightDir;
    //
    // World up direction (standard "up" is positive Y-axis)
    glm::vec3 worldUp;  // World "up" direction (Y-axis)
    // Compute the right direction (perpendicular to both lightDir and worldUp)
    glm::vec3 rightDir;  // Right direction
    // Compute the up direction (perpendicular to both lightDir and rightDir)
    glm::vec3 upDir;  // Up direction
    //


    // @@ Perhaps declare additional scene lighting values here. (lightVal, lightAmb)
    glm::vec3 lightVal;// Brightness for micro-facet BRDF
    glm::vec3 lightAmb;//ambient light
    glm::vec3 lowSpecular;
    
    float RoughSurface;//ground
    float PolishedSurface;//teapot
    float ModerateSmoothSurface;//podium
//   glm::vec3 Light, Ambient;
    glm::vec3 Light = glm::vec3(3.0, 3.0, 3.0);
    glm::vec3 Ambient = glm::vec3(0.2, 0.2, 0.2);//vec3(0.1, 0.1, 0.1);//vec3(0.2, 0.2, 0.2);


    //
    //Texture variables
    Texture*  teapotTexture;
    Texture* floorTexture;
    Texture* podiumTexture;
    Texture* groundTexture;
    Texture* roomTexture;
    Texture* HouseTexture;
    Texture* SkyTexture;

    Texture* FloorNormalT;
    Texture* PodiumNormalT;
    Texture* RipplesNormalT;
    Texture* BricksNormalT;

    HDRTexture* HDRP_SKYTexture;
    HDRTexture* HDRP_IrradianceTxt;
    
    int mode; // Extra mode indicator hooked up to number keys and sent to shader
    
    // Viewport
    int width, height;

    // Transformations
    glm::mat4 WorldProj, WorldView, WorldInverse, L_ModelMatrix, L_ViewMatrix, L_ProjectionMatrix, ShadowMatrix, B;


    // All objects in the scene are children of this single root object.
    Object* objectRoot;
    Object *central, *anim, *room, *floor, *teapot, *podium, *sky,
            *ground, *sea, *spheres, *leftFrame, *rightFrame;

    std::vector<Object*> animated;
    ProceduralGround* proceduralground;

    // Shader programs
    ShaderProgram* lightingProgram;
    // @@ Declare additional shaders if necessary
    ShaderProgram* ShadowProgram;
    FBO* Shadow_Fbo;
    //
    //Reflection Fbos and Prg;
    ShaderProgram* ReflectionProgram;
    FBO* Up_Fbo;
    FBO* Lower_Fbo;
    //
    //
    //Upper reflection map
    //Lower reflection map
    glm::vec3 EYE = glm::vec3(0.0, 0.0, 1.5);


    // Options menu stuff
    bool show_demo_window;

    void InitializeScene();
    void BuildTransforms();
    void DrawMenu();
    void DrawScene();

};
