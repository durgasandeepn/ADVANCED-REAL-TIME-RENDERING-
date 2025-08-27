
////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.


#include "math.h"
#include <iostream>
#include <stdlib.h>

#include <cmath>
#include <random>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <glu.h>                // For gluErrorString

#define STB_IMAGE_WRITE_IMPLEMENTATION

#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>          // For printing GLM objects with to_string

#include "framework.h"
#include "shapes.h"
#include "object.h"
#include "texture.h"
#include "transform.h"
#include "CreateTextue.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "HDRTexture.h"

const bool fullPolyCount = true; // Use false when emulating the graphics pipeline in software


float e = 0;
const float PI = 3.14159f;
const float rad = PI/180.0f;    // Convert degrees to radians

glm::mat4 Identity(1.0);

const float grndSize = 100.0;    // Island radius;  Minimum about 20;  Maximum 1000 or so
const float grndOctaves = 4.0;  // Number of levels of detail to compute
const float grndFreq = 0.03;    // Number of hills per (approx) 50m
const float grndPersistence = 0.03; // Terrain roughness: Slight:0.01  rough:0.05
const float grndLow = -3.0;         // Lowest extent below sea level
const float grndHigh = 5.0;        // Highest extent above sea level

////////////////////////////////////////////////////////////////////////
// This macro makes it easy to sprinkle checks for OpenGL errors
// throughout your code.  Most OpenGL calls can record errors, and a
// careful programmer will check the error status *often*, perhaps as
// often as after every OpenGL call.  At the very least, once per
// refresh will tell you if something is going wrong.
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line scene.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }

// Create an RGB color from human friendly parameters: hue, saturation, value
glm::vec3 HSV2RGB(const float h, const float s, const float v)
{
    if (s == 0.0)
        return glm::vec3(v,v,v);

    int i = (int)(h*6.0) % 6;
    float f = (h*6.0f) - i;
    float p = v*(1.0f - s);
    float q = v*(1.0f - s*f);
    float t = v*(1.0f - s*(1.0f-f));
    if      (i == 0)     return glm::vec3(v,t,p);
    else if (i == 1)  return glm::vec3(q,v,p);
    else if (i == 2)  return glm::vec3(p,v,t);
    else if (i == 3)  return glm::vec3(p,q,v);
    else if (i == 4)  return glm::vec3(t,p,v);
    else   /*i == 5*/ return glm::vec3(v,p,q);
}

////////////////////////////////////////////////////////////////////////
// Constructs a hemisphere of spheres of varying hues
Object* SphereOfSpheres(Shape* SpherePolygons)
{
    Object* ob = new Object(NULL, nullId);
    
    for (float angle=0.0;  angle<360.0;  angle+= 18.0)
        for (float row=0.075;  row<PI/2.0;  row += PI/2.0/6.0) {   
            glm::vec3 hue = HSV2RGB(angle/360.0, 1.0f-2.0f*row/PI, 1.0f);

            Object* sp = new Object(SpherePolygons, spheresId,
                                    hue, glm::vec3(1.0, 1.0, 1.0), 120.0);
            float s = sin(row);
            float c = cos(row);
            ob->add(sp, Rotate(2,angle)*Translate(c,0,s)*Scale(0.075*c,0.075*c,0.075*c));
        }
    return ob;
}

////////////////////////////////////////////////////////////////////////
// Constructs a -1...+1  quad (canvas) framed by four (elongated) boxes
Object* FramedPicture(const glm::mat4& modelTr, const int objectId, 
                      Shape* BoxPolygons, Shape* QuadPolygons, Texture* _texture)
{
    // This draws the frame as four (elongated) boxes of size +-1.0
    float w = 0.05;             // Width of frame boards.
    
    Object* frame = new Object(NULL, nullId);
    Object* ob;
    
    glm::vec3 woodColor(87.0/255.0,51.0/255.0,35.0/255.0);
    ob = new Object(BoxPolygons, frameId,
                    woodColor, glm::vec3(0.2, 0.2, 0.2), 10.0);
    frame->add(ob, Translate(0.0, 0.0, 1.0+w)*Scale(1.0, w, w));
    frame->add(ob, Translate(0.0, 0.0, -1.0-w)*Scale(1.0, w, w));
    frame->add(ob, Translate(1.0+w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));
    frame->add(ob, Translate(-1.0-w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));

    ob = new Object(QuadPolygons, objectId,
                    woodColor, glm::vec3(0.0, 0.0, 0.0), 10.0,_texture);
    frame->add(ob, Rotate(0,90));

    return frame;
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, shape VAOs, and shader programs as well as setting a
// number of other parameters.
//
void Scene::InitializeScene()
{
    CHECKing = true;
    //
    //glEnable(GL_DEPTH_TEST);
    CHECKERROR;
    AlgoNum = 1;
    AlgoNames = { "Initial Shadow Algo","Variance", "MSM" };
    //ShadowTesting = DepthTestTest;
    DepthFlag = true;

    // @@ Initialize interactive viewing variables here. (spin, tilt, ry, front back, ...)
    spin = 0.0f;  // Spin angle
    tilt = 30.0f; // Tilt angle
    tx = 0.0f;    // Translation in x-axis
    ty = 0.0f;    // Translation in y-axis
    zoom = 25.0f; // Zoom factor 25.0f
    rx = ry * width / height; // Recalculate rx based on aspect ratio
    ry = 0.4f;    // Fixed vertical frustum height
    front = 0.1f; // Front clipping plane 0.5f
    back = 5000.0f; // Back clipping plane 5000.0f;
    //
    eye = glm::vec3(0, -20, 0);
    Speed = 10;
    w_down = false;
    a_down = false;
    s_down = false;
    d_down = false;
    transformation_mode = false;//Tab key Task2 and Task3
    time_since_last_refresh = 0;
    step = 0;
    CurrentTime = 0;
    PreviousTime = 0;

 
    //AlgoNum = 1;

    //Shadow Map Parameters
    AOMap_Width = 1024;
    AOMap_Height = 1024;

    ShadowMap_Width = 1024;
    ShadowMap_Height = 1024;
    
    ReflectionMap_Width = 1024;
    ReflectionMap_Height = 1024;

    GBufferMap_Width = 1024;
    GBufferMap_Height = 1024;

    // Set initial light parameters
    lightSpin = 150.0;
    lightTilt = -45.0;
    lightDist = 100.0;
    // @@ Perhaps initialize additional scene lighting values here. (lightVal, lightAmb)
    //
    //LIGHT PARAMETERS
    //
    glm::vec3 lowSpecular(0.03);
    //glm::vec3 lowSpecular(0.3, 0.3, 0.3);
    //Alpha values
    RoughSurface = 3.0f;//ground
    PolishedSurface = 120.0f;//teapot
    ModerateSmoothSurface = 10.0f;//podium

    CHECKERROR;
    objectRoot = new Object(NULL, nullId);
    DS_Root = new Object(NULL, nullId);
    LightSphs_Root = new Object(NULL, nullId);
    

    //This is for ShadowMap
    Fbo = new FBO(ShadowMap_Width, ShadowMap_Height);
    Fbo->CreateFBO(ShadowMap_Width, ShadowMap_Height);
    //Fbo->CreateFBO_Multi(ShadowMap_Width, ShadowMap_Height);

    
    GB_Fbos = new FBO(GBufferMap_Width, GBufferMap_Height);
    GB_Fbos->CreateFBO_Multi(GBufferMap_Width, GBufferMap_Height);


    AmbientFBO = new FBO(AOMap_Width, AOMap_Height);
    AmbientFBO->CreateFBO(AOMap_Width, AOMap_Height);
    
    Up_Fbo = new FBO(ReflectionMap_Width, ReflectionMap_Height);
    Up_Fbo->CreateFBO(ReflectionMap_Width, ReflectionMap_Height);
    Lower_Fbo = new FBO(ReflectionMap_Width, ReflectionMap_Height);
    Lower_Fbo->CreateFBO(ReflectionMap_Width, ReflectionMap_Height);


    //
    //COMPUTE SHADER
    CreateTextureObj = new CreateTexture();
    Project2Methods = new Project2(0);
    Project2Methods->CreateUniformBuffer( );
    //
    //ShaderProgram* HorizCS, VerticalCS;
    HorizCS = new ShaderProgram( );
    HorizCS->AddShader("Horiz.comp", GL_COMPUTE_SHADER);
    HorizCS->LinkProgram();
    CreateTextureObj->CreatingTexture(ShadowMap_Width,ShadowMap_Height,&HorizCS->BlurSMTextureId);
    //
    VerticalCS = new ShaderProgram( );
    VerticalCS->AddShader("Vertical.comp", GL_COMPUTE_SHADER);
    VerticalCS->LinkProgram();
    CreateTextureObj->CreatingTexture(ShadowMap_Width, ShadowMap_Height, &VerticalCS->FinalSMTextureId);
    //

    //
    //COMPUTE SHADER AO
    // 
    //ShaderProgram* AOH_CS;
    //ShaderProgram* AOV_CS;
    CreateTextureObj_AO = new CreateTexture();
    Project2Methods_AO = new Project2(1);
    Project2Methods_AO->CreateUniformBuffer( );
    //
    AOH_CS = new ShaderProgram();
    AOH_CS->AddShader("BilateralHoriz.comp", GL_COMPUTE_SHADER);
    AOH_CS->LinkProgram();
    CreateTextureObj_AO->CreatingTexture(ShadowMap_Width, ShadowMap_Height, &AOH_CS->AOHorzTexture_Id);
    //
    AOV_CS = new ShaderProgram();
    AOV_CS->AddShader("BilateralVertical.comp", GL_COMPUTE_SHADER);
    AOV_CS->LinkProgram();
    CreateTextureObj_AO->CreatingTexture(ShadowMap_Width, ShadowMap_Height, &AOV_CS->AOFinalTexture_Id);
    //
    


    //
    //Shadow Pass
    ShadowProgram = new ShaderProgram();
    ShadowProgram->AddShader("shadow.vert", GL_VERTEX_SHADER);
    ShadowProgram->AddShader("shadow.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(ShadowProgram->programId, 0, "vertex");
    ShadowProgram->LinkProgram();
    //
    //
    //Reflection Pass
    ReflectionProgram = new ShaderProgram();
    ReflectionProgram->AddShader("reflection.vert", GL_VERTEX_SHADER);
    ReflectionProgram->AddShader("reflection.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(ReflectionProgram->programId, 0, "vertex");
    glBindAttribLocation(ReflectionProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(ReflectionProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(ReflectionProgram->programId, 3, "vertexTangent");
    ReflectionProgram->LinkProgram();
    //
    //18-03-2025 --> 25-03-2025
    GBufferProgram = new ShaderProgram();
    GBufferProgram->AddShader("gbuffer.vert", GL_VERTEX_SHADER);
    GBufferProgram->AddShader("gbuffer.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(GBufferProgram->programId, 0, "vertex");
    glBindAttribLocation(GBufferProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(GBufferProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(GBufferProgram->programId, 3, "vertexTangent");
    GBufferProgram->LinkProgram();
    //
    

    //
    AmbientProgram = new ShaderProgram( );
    AmbientProgram->AddShader("AO.vert", GL_VERTEX_SHADER);
    AmbientProgram->AddShader("AO.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(AmbientProgram->programId, 0, "vertex");
    glBindAttribLocation(AmbientProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(AmbientProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(AmbientProgram->programId, 3, "vertexTangent");
    AmbientProgram->LinkProgram( );
    //
    

    //
    // Create the lighting shader program from source code files.
    // @@ Initialize additional shaders if necessary
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("lighting.vert", GL_VERTEX_SHADER);
    lightingProgram->AddShader("lighting.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->programId, 0, "vertex");
    glBindAttribLocation(lightingProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->programId, 2, "vertexTexture");
    //glBindAttribLocation(lightingProgram->programId, 3, "vertexTangent");
    lightingProgram->LinkProgram();

    
    //LocalLights Shader
    LocalLightsProgram = new ShaderProgram();
    LocalLightsProgram->AddShader("locallight.vert", GL_VERTEX_SHADER);
    LocalLightsProgram->AddShader("locallight.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(LocalLightsProgram->programId, 0, "vertex");
    glBindAttribLocation(LocalLightsProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(LocalLightsProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(LocalLightsProgram->programId, 3, "vertexTangent");
    LocalLightsProgram->LinkProgram();

    // Create all the Polygon shapes
    proceduralground = new ProceduralGround(grndSize, 400,
                                     grndOctaves, grndFreq, grndPersistence,
                                     grndLow, grndHigh);
    
    Shape* TeapotPolygons =  new Teapot(fullPolyCount?12:2);
    Shape* BoxPolygons = new Box();
    Shape* SpherePolygons = new Sphere(32);
    Shape* RoomPolygons = new Ply("room.ply");
    Shape* FloorPolygons = new Plane(10.0, 10);
    Shape* QuadPolygons = new Quad();
    Shape* SeaPolygons = new Plane(2000.0, 50);
    Shape* GroundPolygons = proceduralground;
    //
    //FOR DEFERRED SHADING
    Shape* ScreenQuadPolys = new Quad(1);
    //
    //Shape* LightSphere = new Sphere(32, 1);
   
    //LightSpheres.push_back(LightSphere);
    
    // 
    // Various colors used in the subsequent models
    glm::vec3 woodColor(87.0/255.0, 51.0/255.0, 35.0/255.0);
    glm::vec3 brickColor(134.0/255.0, 60.0/255.0, 56.0/255.0);
    glm::vec3 floorColor(6*16/255.0, 5.5*16/255.0, 3*16/255.0);
    glm::vec3 brassColor(0.5, 0.5, 0.1);//(0.03, 0.03, 0.03);
    glm::vec3 grassColor(62.0/255.0, 102.0/255.0, 38.0/255.0);
    glm::vec3 waterColor(0.3, 0.3, 1.0);

    glm::vec3 black(0.0, 0.0, 0.0);
    glm::vec3 brightSpec(0.5, 0.5, 0.5);//0.03
    glm::vec3 polishedSpec(0.3, 0.3, 0.3);//0.03
    //
    // Creates all the models from which the scene is composed.  Each
    // is created with a polygon shape (possibly NULL), a
    // transformation, and the surface lighting parameters Kd, Ks, and
    // alpha.
    //
    // @@ This is where you could read in all the textures and
    // associate them with the various objects being created in the
    // next dozen lines of code.
    teapotTexture = new Texture("textures/cracks.png");
    floorTexture = new Texture("textures/6670-diffuse.jpg");
    podiumTexture = new Texture("textures/Brazilian_rosewood_pxr128.png");
    groundTexture = new Texture("textures/grass.jpg");
    roomTexture = new Texture("textures/Standard_red_pxr128.png");
    HouseTexture = new Texture("textures/my-house-01.png");
    //SkyTexture = new Texture("skys/sky.jpg");
    SkyTexture = new Texture("skys/Ocean.png");


    FloorNormalT = new Texture("textures/6670-normal.jpg");
    PodiumNormalT = new Texture("textures/Brazilian_rosewood_pxr128_normal.png");
    RipplesNormalT = new Texture("textures/ripples_normalmap.png");
    BricksNormalT = new Texture("textures/Standard_red_pxr128_normal.png");

    HDRP_SKYTexture = new HDRTexture("HDRP_Texture/Newport_Loft_Ref.hdr");
    HDRP_IrradianceTxt = new HDRTexture("HDRP_Texture/Newport_Loft_Ref.irr.hdr");

    HDRP_ReadAsPixel = new HDRTexture();
    HDRP_ReadAsPixel->HDR_ReadAsPixel("HDRP_Texture/Newport_Loft_Ref.hdr");
    ReadingSH();



    // @@ To change an object's surface parameters (Kd, Ks, or alpha),
    // modify the following lines.
    central    = new Object(NULL, nullId);
    anim       = new Object(NULL, nullId);
    room       = new Object(RoomPolygons, roomId, brickColor, black, 1, roomTexture, BricksNormalT);//texture
    floor = new Object(FloorPolygons, floorId, floorColor, black, RoughSurface, floorTexture, FloorNormalT);//texture
    teapot = new Object(TeapotPolygons, teapotId, brassColor, lowSpecular, PolishedSurface, teapotTexture);//texture
    podium = new Object(BoxPolygons, boxId, glm::vec3(woodColor), lowSpecular, ModerateSmoothSurface, podiumTexture, PodiumNormalT);//texture
    //sky        = new Object(SpherePolygons, skyId, black, black,0, SkyTexture);
    sky = new Object(SpherePolygons, skyId, black, black, 0, HDRP_SKYTexture);
    objectRoot->add(sky, Scale(2000.0, 2000.0, 2000.0));
    ground = new Object(GroundPolygons, groundId, grassColor, black, RoughSurface, groundTexture);//texture
    //sea        = new Object(SeaPolygons, seaId, waterColor, lowSpecular, PolishedSurface, SkyTexture, RipplesNormalT);
    sea = new Object(SeaPolygons, seaId, waterColor, lowSpecular, PolishedSurface, HDRP_SKYTexture, RipplesNormalT);
    leftFrame  = FramedPicture(Identity, lPicId, BoxPolygons, QuadPolygons, NULL);//No texture
    rightFrame = FramedPicture(Identity, rPicId, BoxPolygons, QuadPolygons,HouseTexture);//texture
    spheres    = SphereOfSpheres(SpherePolygons);

    ScreenQuad = new Object(ScreenQuadPolys, ScreenQuadId);

    //CHECKERROR;

#ifdef REFL
    spheres->drawMe = true;
#else
    spheres->drawMe = false;
#endif


    //spheres->drawMe = true;

    // @@ To change the scene hierarchy, examine the hierarchy created
    // by the following object->add() calls and adjust as you wish.
    // The objects being manipulated and their polygon shapes are
    // created above here.

    // Scene is composed of sky, ground, sea, room and some central models
    if (fullPolyCount) {
        objectRoot->add(sky, Scale(2000.0, 2000.0, 2000.0));
        objectRoot->add(sea); 
        objectRoot->add(ground); }
    objectRoot->add(central);
#ifndef REFL
    objectRoot->add(room,  Translate(0.0, 0.0, 0.02));
#endif
    objectRoot->add(floor, Translate(0.0, 0.0, 0.02));

    // Central model has a rudimentary animation (constant rotation on Z)
    animated.push_back(anim);

    // Central contains a teapot on a podium and an external sphere of spheres
    central->add(podium, Translate(0.0, 0,0));
    central->add(anim, Translate(0.0, 0,0));
    anim->add(teapot, Translate(0,0,1)*Scale(0.31,0.31,0.31));

    if (fullPolyCount)
        anim->add(spheres, Translate(0.0, 0.0, 0.0)*Scale(16, 16, 16));
    
    // Room contains two framed pictures
    if (fullPolyCount) {
        room->add(leftFrame, Translate(-1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8));
        room->add(rightFrame, Translate( 1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8)); }

    

    //FOR DEFERRED SHADING 
    if (fullPolyCount) {
        DS_Root->add(ScreenQuad, glm::mat4(1.0));
    }


    CreatePointLights( );
    CreateSSBO();//creating SSBO
    /*
    for (int i = 0; i < (LightSpheres.size() - 1); i++) {
        Object* LightSphObj = new Object(LightSpheres[i], LightSph);
        LightSphObjs.push_back(LightSphObj);
    }
    */
    
    if (fullPolyCount) {
        glm::vec3 TempPos;
        float TempScale;
        for (int i = 0; i < (pointLights.size() - 1); i++) {
            //
            TempPos = pointLights[i].position;
            TempScale = pointLights[i].radius/12;
            //LightSphs_Root->add(LightSphObj, glm::mat4(1.0));
            LightSphs_Root->add(LightSphObjs[i], Translate(TempPos.x, TempPos.y, 0) * Scale(TempScale, TempScale, TempScale));
            //
        }
    }
    
    //
    // Options menu stuff
    show_demo_window = false;
}

void Scene::DrawMenu()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        // This menu demonstrates how to provide the user a list of toggleable settings.
        if (ImGui::BeginMenu("Objects")) {
            if (ImGui::MenuItem("Draw spheres", "", spheres->drawMe))  {spheres->drawMe ^= true; }
            if (ImGui::MenuItem("Draw walls", "", room->drawMe))       {room->drawMe ^= true; }
            if (ImGui::MenuItem("Draw ground/sea", "", ground->drawMe)){ground->drawMe ^= true;
                							sea->drawMe = ground->drawMe;}
            ImGui::EndMenu(); }
        
        // This menu demonstrates how to provide the user a choice
        // among a set of choices.  The current choice is stored in a
        // variable named "mode" in the application, and sent to the
        // shader to be used as you wish.
        if (ImGui::BeginMenu("Menu ")) {
            if (ImGui::MenuItem("<sample menu of choices>", "",	false, false)) {}
            if (ImGui::MenuItem("Do nothing 0", "",		mode==0)) { mode=0; }
            if (ImGui::MenuItem("Do nothing 1", "",		mode==1)) { mode=1; }
            if (ImGui::MenuItem("Do nothing 2", "",		mode==2)) { mode=2; }
            ImGui::EndMenu(); }
        

    ImGui::EndMainMenuBar(); }  
    ImGui::GetIO().FontGlobalScale = 1.5f;
    ImGui::Text("No.of Lights");
    ImGui::SliderFloat("", &Value, 1.0f, 1000.0f, "%.0f", 1.0f);

    ImGui::Separator();
    ImGui::Separator();
    //
    //KernalSize
    ImGui::Text("Kernal Value: %d", Project2Methods->KernalValue());
    if (ImGui::Button("Increase")) {
        Project2Methods->IncrementKernal( );
    }
    if (ImGui::Button("Decrease")) {
        
        Project2Methods->DecrementKernal( );
    }

    ImGui::Separator();
    ImGui::Separator();

   // ImGui::Text("Algo Value: %d", AlgoNum);
    ImGui::Text("AlgoNum Value: %d", AlgoNum);
    if (ImGui::Button("<")) {

        if (AlgoNum > 0 && AlgoNum < 3) {
            AlgoNum--;
        }
        
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(AlgoNames[(AlgoNum)].c_str());
    ImGui::SameLine();
    if (ImGui::Button(">")) {

        if (AlgoNum >= 0 && AlgoNum < 2) {
            AlgoNum++;
        }
    }

    ImGui::Separator();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::BuildTransforms()
{
    // @@ When you are ready to try interactive viewing, replace the
    // following hard coded values for WorldProj and WorldView with
    // transformation matrices calculated from variables such as spin,
    // tilt, tr, ry, front, and back.
   
    rx = ry * ((float)width / (float)height);
    if (transformation_mode == false) {
    
        WorldView = Translate(tx, ty, -zoom) * Rotate(0, tilt - 90) * Rotate(2, spin);
        WorldProj = Perspective(rx, ry, front, back);
    }
    else  if (transformation_mode == true)
    {
        WorldView = Rotate(0,(tilt - 90)) * Rotate(2,spin) * Translate(-eye.x, -eye.y, -eye.z);
        WorldProj = Perspective(rx,ry, front, back);
    }

    //
    //
    // LIGHT CALCULATIONS //
    //
    lightDir = glm::normalize(lightPos);// 0 -LightPos
    // World up direction (standard "up" is positive Y-axis)
    worldUp = glm::vec3(0.0f, 0.0f, 1.0f);  // World "up" direction (Y-axis)
    // Compute the right direction (perpendicular to both lightDir and worldUp)
    rightDir = glm::normalize(glm::cross(worldUp, lightDir));  // Right direction
    // Compute the up direction (perpendicular to both lightDir and rightDir)
    upDir = glm::normalize(glm::cross(lightDir, rightDir));  // Up direction
    //
    L_ViewMatrix = LookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), worldUp);
    /*
    std::cout << glm::to_string(L_ViewMatrix) << std::endl;
    std::cout<<std::endl;
    std::cout<<std::endl;
    std::cout << glm::to_string(glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), worldUp)) << std::endl;
    */

//  L_ViewMatrix = LookAt(lightPos, lightDir, upDir);

    Rx = 40/lightDist;
    Ry = 40/lightDist;
    
    L_ProjectionMatrix = Perspective(Rx, Ry, 10, 1000);//back -> 5000
    //std::cout << glm::to_string(L_ProjectionMatrix) << std::endl;
    //
    B = Translate(0.5, 0.5, 0.5) * Scale(0.5, 0.5, 0.5);
    ShadowMatrix = B * L_ProjectionMatrix * L_ViewMatrix;
    //
    //
      
    //
    //
    // @@ Print the two matrices (in column-major order) for
    // comparison with the project document.
    /*
    std::cout << "WorldView: " << glm::to_string(WorldView) << std::endl;
    std::cout << "WorldProj: " << glm::to_string(WorldProj) << std::endl;
    */
    
}


void Scene::CreatePointLights() {

    unsigned int i = 0;
    unsigned int j = 0;

    float  Xpos = -20.0f;
    float Ypos = -20.0f; 
    float temp = Ypos;
    
   // Xpos = 0;
   // Ypos = 0;
   // temp = Ypos;

    //
    //position starts from 
    // -5 to -50
    //

    //for (i = 0; i <4; i++) {
      // for (j = 0; j < 4; j++) {

    for (i = 0; i < 40; i++) {
       for (j = 0; j < 25; j++) {
            
            PointLightData LightsData;

            LightsData.position = glm::vec3(Xpos, Ypos, 0);
            LightsData.color = RandomColor( );
            LightsData.intensity = RandomIntensity( );
            LightsData.radius = RandomRadius( );
            //LightsData.radius = 2;
            
            pointLights.push_back(LightsData);

            Shape* LightSphere = new Sphere(32, LightsData.radius);
            LightSpheres.push_back(LightSphere);//LightSpheres is vector for storing Shape

            Object* LightSphObj = new Object(LightSphere, LightSph);
            LightSphObjs.push_back(LightSphObj);
            
            Ypos += 1.5f;

        }
            Xpos += 1.5f;
            Ypos = temp;

    }
    
}


glm::vec3 Scene::RandomColor( ) {

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0f, 1.0f);

    float r = dis(gen);
    float g = dis(gen);
    float b = dis(gen);

    // Round to 1 decimal place
    r = std::round(r * 10.0f) / 10.0f;
    g = std::round(g * 10.0f) / 10.0f;
    b = std::round(b * 10.0f) / 10.0f;


    return glm::vec3(r, g, b);

}



glm::vec3 Scene::RandomIntensity( ) {

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.5f, 3.0f);

    float r = dis(gen);
    float g = dis(gen);
    float b = dis(gen);

    // Round to 2 decimal places
    r = std::round(r * 100.0f) / 100.0f;
    g = std::round(g * 100.0f) / 100.0f;
    b = std::round(b * 100.0f) / 100.0f;

    //std::cout << "Random Color is  " << r <<" "<<g <<" " <<b << std::endl;
    return glm::vec3(r, g, b);

}


int Scene::RandomRadius() {

    const int minRadius = 3, maxRadius = 4;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(minRadius, maxRadius);

    return dis(gen);
}


////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be
// drawn. (Which is often: 30 to 60 times per second are the common
// goals.)
const float pi = 3.14159f;
void Scene::DrawScene()
{
    // Set the viewport
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    CurrentTime = glfwGetTime();
    time_since_last_refresh = CurrentTime - PreviousTime;
    step = Speed * time_since_last_refresh;

    if (w_down == true) {
        eye += step * glm::vec3(sin(spin * (pi / 180)), cos(spin * (pi / 180)), 0.0f);
    }
    if(s_down == true)
    {
        eye -= step * glm::vec3(sin(spin * (pi / 180)), cos(spin * (pi / 180)), 0.0f);
    }
    if(d_down == true)
    {
        eye += step * glm::vec3(cos(spin * (pi / 180)), -sin(spin * (pi / 180)), 0.0f);
    }
    if(a_down == true)
    {
        eye -= step * glm::vec3(cos(spin * (pi / 180)), -sin(spin * (pi / 180)), 0.0f);
    }
    

    //CHECKERROR;
    // Calculate the light's position from lightSpin, lightTilt, lightDist
    lightPos = glm::vec3(lightDist*cos(lightSpin*rad)*sin(lightTilt*rad),
                         lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
                         lightDist*cos(lightTilt*rad));


    //
    float DistLightToOrigin = glm::length(lightPos);
    z_near = DistLightToOrigin - 40;
    z_far = DistLightToOrigin + 40;


    // Update position of any continuously animating objects
    double atime = 360.0*glfwGetTime()/36;
    for (std::vector<Object*>::iterator m=animated.begin();  m<animated.end();  m++)

        (*m)->animTr = Rotate(2, atime);


    eye.z = 2 + proceduralground->HeightAt(eye.x, eye.y);;

    BuildTransforms();

    // The lighting algorithm needs the inverse of the WorldView matrix
    WorldInverse = glm::inverse(WorldView);
    

    ////////////////////////////////////////////////////////////////////////////////
    // Anatomy of a pass:
    //   Choose a shader  (create the shader in InitializeScene above)
    //   Choose and FBO/Render-Target (if needed; create the FBO in InitializeScene above)
    //   Set the viewport (to the pixel size of the screen or FBO)
    //   Clear the screen.
    //   Set the uniform variables required by the shader
    //   Draw the geometry
    //   Unset the FBO (if one was used)
    //   Unset the shader
    ////////////////////////////////////////////////////////////////////////////////

    //CHECKERROR;
    int loc, programId, ShadowPrgId, GBufferPrgId, LocalLightsPrgId, HorzPrjId, VerticalPrjId, AOProgramId, AOHorizPrjId, AOVerticalPrjId;

    ////////////////////////////////////////////////////////////////////////////////
    // G-Buffer pass - STARTING
    ////////////////////////////////////////////////////////////////////////////////
    glEnable(GL_DEPTH_TEST);

    GB_Fbos->BindFBO();
    //
    glViewport(0, 0, GBufferMap_Width, GBufferMap_Height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //
    GBufferProgram->UseShader( );
    GBufferPrgId = GBufferProgram->programId;
    //
    loc = glGetUniformLocation(GBufferPrgId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(GBufferPrgId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(GBufferPrgId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
    loc = glGetUniformLocation(GBufferPrgId, "lightPos");
    glUniform3fv(loc, 1, &(lightPos[0]));
    loc = glGetUniformLocation(GBufferPrgId, "mode");
    glUniform1i(loc, mode);
    loc = glGetUniformLocation(GBufferPrgId, "Light");
    glUniform3fv(loc, 1, &(Light[0]));
    loc = glGetUniformLocation(GBufferPrgId, "Ambient");
    glUniform3fv(loc, 1, &(Ambient[0]));
    loc = glGetUniformLocation(GBufferPrgId, "ShadowMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));

    objectRoot->Draw(GBufferProgram, Identity);

    GBufferProgram->UnuseShader();

    GB_Fbos->UnbindFBO();
    //
    ////////////////////////////////////////////////////////////////////////////////
    // G-Buffer pass - ENDING
    ////////////////////////////////////////////////////////////////////////////////
    //if (CHECKing == false) {
    

    ////////////////////////////////////////////////////////////////////////////////
    // AO pass - STARTING
    ////////////////////////////////////////////////////////////////////////////////

    AmbientFBO->BindFBO();

    glViewport(0, 0, GBufferMap_Width, GBufferMap_Height);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    AmbientProgram->UseShader();
    AOProgramId = AmbientProgram->programId;

    loc = glGetUniformLocation(AOProgramId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(AOProgramId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(AOProgramId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));

    GB_Fbos->BindTexture4(3, AOProgramId, "WorldPosMap");
    GB_Fbos->BindTexture4(4, AOProgramId, "NMap");
    GB_Fbos->BindTexture4(5, AOProgramId, "KdMap");
    GB_Fbos->BindTexture4(6, AOProgramId, "KaMap");

    DS_Root->Draw(AmbientProgram, Identity);

    AmbientProgram->UnuseShader();

    AmbientFBO->UnbindFBO();

    ////////////////////////////////////////////////////////////////////////////////
    // AO pass - ENDING
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // AO - Compute Shader Start
    ////////////////////////////////////////////////////////////////////////////////
    
    //
    //Ambient Horizontal 
    AOH_CS->UseShader_CS(0);
    AOHorizPrjId = AOH_CS->programId;
    //Fbo->BindTexture_CS(0, HorzPrjId, "shadowMap");//
    AmbientFBO->BindImageTexture_CS(0, GL_READ_ONLY, GL_RGBA32F, AOHorizPrjId, "AOMap");
    AOH_CS->BindImageTexture(1, AOH_CS->AOHorzTexture_Id, GL_WRITE_ONLY, AOHorizPrjId, "AOBlurH");
    //
    GB_Fbos->BindImageTexture_AO(2, GL_READ_WRITE, GL_RGBA32F, AOHorizPrjId, "WorldPosMap");
    GB_Fbos->BindImageTexture_AO(3, GL_READ_WRITE, GL_RGBA32F, AOHorizPrjId, "NMap");

    //
    //UnifromBuffer
    loc = glGetUniformBlockIndex(AOHorizPrjId, "blurKernal");
    glUniformBlockBinding(AOHorizPrjId, loc, Project2Methods_AO->bindpointAO);

    GLint uniformLocation0 = glGetUniformLocation(AOHorizPrjId, "blurWidth");
    glUniform1i(uniformLocation0, Project2Methods_AO->blurWidth);
    //
    AOH_CS->DispatchComputerShader(1024, 1024, 128, 1);

    //
    //Ambient Vertical 
    AOV_CS->UseShader_CS(0);
    AOVerticalPrjId = AOV_CS->programId;
    AOV_CS->BindImageTexture(0, AOH_CS->AOHorzTexture_Id, GL_READ_ONLY, AOVerticalPrjId, "AOBlurH");
    AOV_CS->BindImageTexture(1, AOV_CS->AOFinalTexture_Id, GL_WRITE_ONLY, AOVerticalPrjId, "AOFinalBlur");
    //
    GB_Fbos->BindImageTexture_AO(2, GL_READ_WRITE, GL_RGBA32F, AOVerticalPrjId, "WorldPosMap");
    GB_Fbos->BindImageTexture_AO(3, GL_READ_WRITE, GL_RGBA32F, AOVerticalPrjId, "NMap");

    loc = glGetUniformBlockIndex(AOVerticalPrjId, "blurKernal");
    glUniformBlockBinding(AOVerticalPrjId, loc, Project2Methods_AO->bindpointAO);

    GLint uniformLocation01 = glGetUniformLocation(AOVerticalPrjId, "blurWidth");
    glUniform1i(uniformLocation01, Project2Methods_AO->blurWidth);

    AOV_CS->DispatchComputerShader(1024, 1024, 1, 128);

    ////////////////////////////////////////////////////////////////////////////////
    // AO -  Compute Shader End
    ////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////
        // Shadow pass
        ////////////////////////////////////////////////////////////////////////////////
        //Create FBO
        //
        glEnable(GL_DEPTH_TEST);
        // Set the viewport, and clear the screen 
        Fbo->BindFBO();

        //
        glViewport(0, 0, ShadowMap_Width, ShadowMap_Height);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //
        //
        // Choose the lighting shader
        ShadowProgram->UseShader();
        ShadowPrgId = ShadowProgram->programId;

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        glUniform1f(glGetUniformLocation(ShadowPrgId, "z0"), z_near);
        glUniform1f(glGetUniformLocation(ShadowPrgId, "z1"), z_far);

        loc = glGetUniformLocation(ShadowPrgId, "L_ViewMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(L_ViewMatrix));
        loc = glGetUniformLocation(ShadowPrgId, "L_ProjectionMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(L_ProjectionMatrix));


        //CHECKERROR;
        // Draw all objects (This recursively traverses the object hierarchy.)
        objectRoot->Draw(ShadowProgram, Identity);
        //CHECKERROR;

        glDisable(GL_CULL_FACE);

        // Turn off the shader
        ShadowProgram->UnuseShader();
        //For the DepthTest
        Fbo->UnbindFBO();
        //
        //
        ////////////////////////////////////////////////////////////////////////////////
        // End of Shadow pass
        ////////////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////////////
        // Compute Shader
        ////////////////////////////////////////////////////////////////////////////////
        
        HorizCS->UseShader_CS(0);
        HorzPrjId = HorizCS->programId;
        //Fbo->BindTexture_CS(0, HorzPrjId, "shadowMap");//
        Fbo->BindImageTexture_CS(0,GL_READ_ONLY, GL_RGBA32F, HorzPrjId, "shadowMap");//
        //Fbo->BindImageTexture_CS(1, GL_WRITE_ONLY, GL_RGBA32F, HorzPrjId, "blurredShadowMap");//
        HorizCS->BindImageTexture(1, HorizCS->BlurSMTextureId, GL_WRITE_ONLY,HorzPrjId,"blurredShadowMap");

        GB_Fbos->BindImageTexture_AO(2, GL_READ_ONLY, GL_RGBA32F, HorzPrjId, "WorldPosMap");
        GB_Fbos->BindImageTexture_AO(3, GL_READ_ONLY, GL_RGBA32F, HorzPrjId, "NMap");


        //UnifromBuffer
        loc = glGetUniformBlockIndex(HorzPrjId, "blurKernal");
        glUniformBlockBinding(HorzPrjId, loc, Project2Methods->bindpoint);

        GLint uniformLocation1 = glGetUniformLocation(HorzPrjId, "blurWidth");
        glUniform1i(uniformLocation1, Project2Methods->blurWidth);
        //
        HorizCS->DispatchComputerShader(1024, 1024, 128, 1);

        VerticalCS->UseShader_CS(0);
        VerticalPrjId = VerticalCS->programId;
        VerticalCS->BindImageTexture(0,HorizCS->BlurSMTextureId, GL_READ_ONLY, VerticalPrjId, "blurredShadowMap");
        VerticalCS->BindImageTexture(1,VerticalCS->FinalSMTextureId, GL_WRITE_ONLY, VerticalPrjId, "finalShadowMap");
        
        loc = glGetUniformBlockIndex(VerticalPrjId, "blurKernal");
        glUniformBlockBinding(VerticalPrjId, loc, Project2Methods->bindpoint);

        GLint uniformLocation = glGetUniformLocation(VerticalPrjId, "blurWidth");
        glUniform1i(uniformLocation, Project2Methods->blurWidth);

        
        //VerticalCS->DispatchComputerShader(ShadowMap_Width, ShadowMap_Height);
        VerticalCS->DispatchComputerShader(1024, 1024, 1, 128);
        
        ////////////////////////////////////////////////////////////////////////////////
        // Compute Shader
        ////////////////////////////////////////////////////////////////////////////////





    ////////////////////////////////////////////////////////////////////////////////
    // Reflections pass
    ////////////////////////////////////////////////////////////////////////////////
    //Upper Fbo
        room->drawMe = false;
        teapot->drawMe = false;
        ground->drawMe = false;
        sea->drawMe = false;
        //
        ReflectionProgram->UseShader();
        programId = ReflectionProgram->programId;
        //
        Up_Fbo->BindFBO();
        //
        glViewport(0, 0, ReflectionMap_Width, ReflectionMap_Height);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //
        loc = glGetUniformLocation(programId, "WorldProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
        loc = glGetUniformLocation(programId, "WorldView");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
        loc = glGetUniformLocation(programId, "WorldInverse");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
        loc = glGetUniformLocation(programId, "lightPos");
        glUniform3fv(loc, 1, &(lightPos[0]));
        loc = glGetUniformLocation(programId, "mode");
        glUniform1i(loc, mode);

        //
        loc = glGetUniformLocation(programId, "Light");
        glUniform3fv(loc, 1, &(Light[0]));
        loc = glGetUniformLocation(programId, "Ambient");
        glUniform3fv(loc, 1, &(Ambient[0]));
        loc = glGetUniformLocation(programId, "ShadowMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));
        //
        Fbo->BindTexture(2, programId, "shadowMap");
        //
        loc = glGetUniformLocation(programId, "SIGN");
        glUniform1f(loc, 1.0);//upper map
        //
        loc = glGetUniformLocation(programId, "FLAG");
        glUniform1f(loc, true);//upper map

        // Draw all objects (This recursively traverses the object hierarchy.)
        objectRoot->Draw(ReflectionProgram, Identity);

        //Shadow_Fbo->UnbindTexture(2);

        Up_Fbo->UnbindFBO();

        //Reflection Pass 2 for 
        //Lower Fbo 
        //

        Lower_Fbo->BindFBO();
        glViewport(0, 0, ReflectionMap_Width, ReflectionMap_Height);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        loc = glGetUniformLocation(programId, "SIGN");
        glUniform1f(loc, -1.0);//lower map

        loc = glGetUniformLocation(programId, "FLAG");
        glUniform1f(loc, false);//upper map

        //Shadow_Fbo->BindTexture(2, programId, "shadowMap");

        objectRoot->Draw(ReflectionProgram, Identity);
        //CHECKERROR
        Fbo->UnbindTexture(2);
        //CHECKERROR
            Lower_Fbo->UnbindFBO();
       // CHECKERROR
            // Turn off the shader
            ReflectionProgram->UnuseShader();
        //CHECKERROR
        ////////////////////////////////////////////////////////////////////////////////
        // End of Reflections pass
        ////////////////////////////////////////////////////////////////////////////////


        //if (DepthFlag == true){

        ////////////////////////////////////////////////////////////////////////////////
        // Lighting pass
        ////////////////////////////////////////////////////////////////////////////////
        teapot->drawMe = true;
        glDisable(GL_DEPTH_TEST);

        // Choose the lighting shader
        lightingProgram->UseShader();
        programId = lightingProgram->programId;

        // Set the viewport, and clear the screen
        glViewport(0, 0, width, height);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // @@ The scene specific parameters (uniform variables) used by
        // the shader are set here.  Object specific parameters are set in
        // the Draw procedure in object.cpp
        
        glUniform1f(glGetUniformLocation(programId, "z0"), z_near);
        glUniform1f(glGetUniformLocation(programId, "z1"), z_far);


        GLint uniformLocation3 = glGetUniformLocation(programId, "AlgoNum");
        glUniform1i(uniformLocation3, AlgoNum);


        //
        Fbo->BindTexture(2, programId, "shadowMap");//
        GB_Fbos->BindTexture4(3, programId, "WorldPosMap");
        GB_Fbos->BindTexture4(4, programId, "NMap");
        GB_Fbos->BindTexture4(5, programId, "KdMap");
        GB_Fbos->BindTexture4(6, programId, "KaMap");
        //VerticalCS->BindTexture(7, programId, "BlurredShadowMap");
        CreateTextureObj->BindTexture(7, programId, "BlurredShadowMap", VerticalCS->FinalSMTextureId);
        CreateTextureObj->BindTexture(8, programId, "AOFinalBlurMap", AOV_CS->AOFinalTexture_Id);

        //VerticalCS->BindImageTexture(7, VerticalCS->BlurredShadowMapTextureId, );//From here
        Up_Fbo->BindTexture(9, programId, "UpMap");
        Lower_Fbo->BindTexture(10, programId, "LowerMap");
        HDRP_IrradianceTxt->BindTexture(11, programId, "IrradianceMap");
        HDRP_SKYTexture->BindTexture(12, programId, "SkyMapHDR");
        //
        
        loc = glGetUniformLocation(programId, "WorldProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
        loc = glGetUniformLocation(programId, "WorldView");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
        loc = glGetUniformLocation(programId, "WorldInverse");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
        loc = glGetUniformLocation(programId, "lightPos1");
        glUniform3fv(loc, 1, &(lightPos[0]));
        loc = glGetUniformLocation(programId, "mode");//not using
        glUniform1i(loc, mode);
        loc = glGetUniformLocation(programId, "Light");//light position
        glUniform3fv(loc, 1, &(Light[0]));
        loc = glGetUniformLocation(programId, "Ambient");//
        glUniform3fv(loc, 1, &(Ambient[0]));
        loc = glGetUniformLocation(programId, "ShadowMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));
        

        /*
        glActiveTexture(GL_TEXTURE2); // Activate texture unit 2
        glBindTexture(GL_TEXTURE_2D, Fbo->GetShadowMapTexture()); // Load texture into it
        loc = glGetUniformLocation(programId, "shadowMap");
        glUniform1i(loc, 2);
        */

        //CHECKERROR;
        // Draw all objects (This recursively traverses the object hierarchy.)
        //objectRoot->Draw(lightingProgram, Identity);
        DS_Root->Draw(lightingProgram, Identity);

        Fbo->UnbindTexture(2);
        //GB_Fbos->UnbindTexture(3);
        //GB_Fbos->UnbindTexture(4);
        //GB_Fbos->UnbindTexture(5);
        //GB_Fbos->UnbindTexture(6);
        //
        //Fbo->UnbindFBO();

        // Turn off the shader
        lightingProgram->UnuseShader();
        ////////////////////////////////////////////////////////////////////////////////
        // End of Lighting pass
        ////////////////////////////////////////////////////////////////////////////////
    //}

    //    //
    //    ////////////////////////////////////////////////////////////////////////////////
    //    // Start of Local lights pass
    //    ////////////////////////////////////////////////////////////////////////////////
    //    //
    //    glDisable(GL_DEPTH_TEST);
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_ONE, GL_ONE);
    //    //
    //    glEnable(GL_CULL_FACE);
    //    //glCullFace(GL_BACK);
    //    glCullFace(GL_FRONT);

    //    LocalLightsProgram->UseShader();
    //    LocalLightsPrgId = LocalLightsProgram->programId;
    //    
    //    /*
    //    glViewport(0, 0, width, height);
    //    glClearColor(0.5, 0.5, 0.5, 1.0);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    */
    //    //
    //    GB_Fbos->BindTexture4(3, LocalLightsPrgId, "WorldPosMap");
    //    GB_Fbos->BindTexture4(4, LocalLightsPrgId, "NMap");
    //    GB_Fbos->BindTexture4(5, LocalLightsPrgId, "KdMap");
    //    GB_Fbos->BindTexture4(6, LocalLightsPrgId, "KaMap");
    //    //
    //    loc = glGetUniformLocation(LocalLightsPrgId, "WorldProj");
    //    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    //    loc = glGetUniformLocation(LocalLightsPrgId, "WorldView");
    //    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    //    loc = glGetUniformLocation(LocalLightsPrgId, "WorldInverse");
    //    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
    //    //
    //    // 
    //    //loc = glGetUniformLocation(LocalLightsPrgId, "Light");
    //    //glUniform3fv(loc, 1, &(Light[0]));
    //    //loc = glGetUniformLocation(LocalLightsPrgId, "Ambient");
    //    //glUniform3fv(loc, 1, &(Ambient[0]));
    //    
    //    loc = glGetUniformLocation(programId, "Ambient");//
    //    glUniform3fv(loc, 1, &(Ambient[0]));
    //    //

    //    for (int i = 0; i < pointLights.size(); i++) {

    //        glUniform1i(glGetUniformLocation(LocalLightsPrgId, "lightIndex"), i);

    //        LightSphs_Root->Draw(LocalLightsProgram, Identity,i);

    //    }


    //    glDisable(GL_BLEND);
    //    glDisable(GL_CULL_FACE);
    //    
    //    LocalLightsProgram->UseShader();

    //    //
    //    ////////////////////////////////////////////////////////////////////////////////
    //    // End of Local lights pass
    //    ////////////////////////////////////////////////////////////////////////////////
    //    //

    //}//CHECKing
    PreviousTime = glfwGetTime();
}



void Scene::PrjWid2025() {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a window for the sidebar
        ImGui::Begin("Sidebar", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Set the width of the sidebar
        ImGui::SetWindowSize(ImVec2(200, 0));

        // Create buttons or menu items in the sidebar
        if (ImGui::Button("Home"))
        {
            // Handle the Home button action
        }
        if (ImGui::Button("Settings"))
        {
            // Handle the Settings button action
        }
        if (ImGui::Button("About"))
        {
            // Handle the About button action
        }

        // You can add more buttons or elements here as needed

        ImGui::End(); // End the sidebar window

        // Now you can render the rest of your UI here, such as the content area
        ImGui::Begin("Main Content");
        ImGui::Text("This is the main content area!");
        ImGui::End();

        ImGui::EndFrame();
}


//SSBO related code
void Scene::CreateSSBO( ) {

    // Generate and bind SSBO
    glGenBuffers(1, &ssboID);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    
    //size_t lightsSize = pointLights.size()
    size_t lightsSize = pointLights.size() * sizeof(PointLightData);
    glBufferData(GL_SHADER_STORAGE_BUFFER, lightsSize, pointLights.data(), GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboID);//binding value is the one that binds
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
    //CHECKERROR;
}


void Scene::PopulateSSBOFromTextures() {
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);

    //// Create a vector to store texture data
    //std::vector<glm::vec4> textureData(4);

    //// Bind each texture and read its data
    //for (int i = 0; i < 4; ++i) {
    //    glBindTexture(GL_TEXTURE_2D, textureIDs[i]); // Use your texture ID array
    //    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &textureData[i]);
    //}

    //// Upload data to SSBO
    //glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizeof(glm::vec4), textureData.data());

    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind SSBO
    //CHECKERROR;
}


void Scene::LoadTextureIDs() {

   /* textureIDs.push_back(3);
    textureIDs.push_back(4);
    textureIDs.push_back(5);
    textureIDs.push_back(6);*/

}



void Scene::ReadingSH() {

    const int width = 400;
    const int height = 200;
    float* Data = new float[width * height * HDRP_ReadAsPixel->channels];
    float DeltaTheta;
    float DeltaThea;

    CHECKERROR
        //
        //HDRP_ReadAsPixel->data
        glm::vec3 L;
    const float pi = 3.14159f;

    float Theta, Thea;
    glm::vec3 ValuesXYZ;

    glm::vec3 Llm[9];
    std::array<float, 9> SHArray;
    glm::vec3 NewLlm[9];

    float area = 0.0f;
    CHECKERROR
        for (int i = 0; i < HDRP_ReadAsPixel->height; i++) {

            for (int j = 0; j < HDRP_ReadAsPixel->width; j++) {

                L.r = HDRP_ReadAsPixel->data[HDRP_ReadAsPixel->channels * (i * HDRP_ReadAsPixel->width + j) + 0];
                L.g = HDRP_ReadAsPixel->data[HDRP_ReadAsPixel->channels * (i * HDRP_ReadAsPixel->width + j) + 1];
                L.b = HDRP_ReadAsPixel->data[HDRP_ReadAsPixel->channels * (i * HDRP_ReadAsPixel->width + j) + 2];


                Theta = (pi) * (i + (1.0f / 2.0f)) / HDRP_ReadAsPixel->height;
                Thea = (2 * pi) * (j + (1.0f / 2.0f)) / HDRP_ReadAsPixel->width;

                CHECKERROR
                    ValuesXYZ.x = glm::cos(Thea) * glm::sin(Theta);
                ValuesXYZ.y = glm::sin(Thea) * glm::sin(Theta);
                ValuesXYZ.z = glm::cos(Theta);

                ValuesXYZ = glm::normalize(ValuesXYZ);

                SHArray = GetSH(ValuesXYZ);

                DeltaTheta = pi / HDRP_ReadAsPixel->height;
                DeltaThea = (2 * pi) / HDRP_ReadAsPixel->width;

                CHECKERROR
                    for (int k = 0; k < 9; k++) {
                        Llm[k] += L * SHArray[k] * glm::sin(Theta) * DeltaTheta * DeltaThea;
                    }
                area += glm::sin(Theta) * DeltaTheta * DeltaThea;
            }
        }

    //
    CHECKERROR
        float AValues[9] = {
            pi,

            (pi * 2.0f / 3.0f),
            (pi * 2.0f / 3.0f),
            (pi * 2.0f / 3.0f),

            (pi * 1.0f / 4.0f),
            (pi * 1.0f / 4.0f),
            (pi * 1.0f / 4.0f),
            (pi * 1.0f / 4.0f),
            (pi * 1.0f / 4.0f)
    };

    CHECKERROR
        for (int h = 0;h < 9; h++) {
            Elm[h] = AValues[h] * Llm[h];
        }

    CHECKERROR



        for (int i = 0; i < height; i++) {

            for (int j = 0; j < width; j++) {

                Theta = (pi) * (i + (1.0f / 2)) / height;
                Thea = (2 * pi) * (j + (1.0f / 2)) / width;

                ValuesXYZ.x = glm::cos(Thea) * glm::sin(Theta);
                ValuesXYZ.y = glm::sin(Thea) * glm::sin(Theta);
                ValuesXYZ.z = glm::cos(Theta);

                ValuesXYZ = glm::normalize(ValuesXYZ);

                SHArray = GetSH(ValuesXYZ);

                DeltaTheta = pi / height;
                DeltaThea = (2 * pi) / width;

                glm::vec3 outputPixel(0.0f);

                for (int k = 0; k < 9; k++) {
                    outputPixel += Elm[k] * SHArray[k];
                }


                Data[HDRP_ReadAsPixel->channels * (i * width + j) + 0] = outputPixel.r;
                Data[HDRP_ReadAsPixel->channels * (i * width + j) + 1] = outputPixel.g;
                Data[HDRP_ReadAsPixel->channels * (i * width + j) + 2] = outputPixel.b;
            }

        }


    stbi_write_hdr("HDRP_Texture/NewImage.hdr", width, height, HDRP_ReadAsPixel->channels, Data);

}


std::array<float, 9> Scene::GetSH(const glm::vec3& direction) {

    float x = direction.x;
    float y = direction.y;
    float z = direction.z;

    std::array<float, 9> base;

    base[0] = 1.0f * SphercalH_Fact[0];
    base[1] = y * SphercalH_Fact[1];
    base[2] = z * SphercalH_Fact[2];
    base[3] = x * SphercalH_Fact[3];
    base[4] = x * y * SphercalH_Fact[4];
    base[5] = y * z * SphercalH_Fact[5];
    base[6] = (3.0f * z * z - 1.0f) * SphercalH_Fact[6];
    base[7] = x * z * SphercalH_Fact[7];
    base[8] = (x * x - y * y) * SphercalH_Fact[8];

    return base;
}


void Scene::PseudoRandomPts() {
    /*
    int n = 20;
    block.N = n;
    int Size = 2 * block.N;
    block.hammersley[Size];

    int kk;
    int pos = 0;
    float u = 0;

    for (int k = 0; k < n; k++) {
        for (float p = 0.5f, kk = k, u = 0.0f; kk; p *= 0.5f, kk >>= 1) {

            if (kk & 1) {
                u += p;
            }

            float v = (k + 0.5) / n;
            block.hammersley[pos++] = u;
            block.hammersley[pos++] = v;
        }
    }
    */
}

