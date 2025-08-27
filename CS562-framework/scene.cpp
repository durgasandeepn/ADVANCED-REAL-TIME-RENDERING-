
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

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <glu.h>                // For gluErrorString

#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>          // For printing GLM objects with to_string

#include "framework.h"
#include "shapes.h"
#include "object.h"
#include "texture.h"
#include "HDRTexture.h"
#include "transform.h"
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
void Scene::InitializeScene()
{
    //
    glEnable(GL_DEPTH_TEST);
    CHECKERROR;

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
    transformation_mode = false;//Tab key Tassk2 and Task3
    time_since_last_refresh = 0;
    step = 0;
    CurrentTime = 0;
    PreviousTime = 0;

    //Shadow Map Parameters
    ShadowMap_Width = 1024;
    ShadowMap_Height = 1024;
    //Reflection Map Parameters
    ReflectionMap_Width = 1024;
    ReflectionMap_Height = 1024;


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
    
    // Enable OpenGL depth-testing
    glEnable(GL_DEPTH_TEST);

    Shadow_Fbo = new FBO(ShadowMap_Width, ShadowMap_Height);
    Shadow_Fbo->CreateFBO(ShadowMap_Width, ShadowMap_Height);

    Up_Fbo = new FBO(ReflectionMap_Width, ReflectionMap_Height);
    Up_Fbo->CreateFBO(ReflectionMap_Width, ReflectionMap_Height);
    Lower_Fbo = new FBO(ReflectionMap_Width, ReflectionMap_Height);
    Lower_Fbo->CreateFBO(ReflectionMap_Width, ReflectionMap_Height);


    //
    //Shadow Pass
    ShadowProgram = new ShaderProgram();
    ShadowProgram->AddShader("shadow.vert", GL_VERTEX_SHADER);
    ShadowProgram->AddShader("shadow.frag", GL_FRAGMENT_SHADER);
    glBindAttribLocation(ShadowProgram->programId, 0, "vertex");
    ShadowProgram->LinkProgram();

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
    //////
    //

    // Create the lighting shader program from source code files.
    // @@ Initialize additional shaders if necessary
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("lighting.vert", GL_VERTEX_SHADER);
    lightingProgram->AddShader("lighting.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->programId, 0, "vertex");
    glBindAttribLocation(lightingProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(lightingProgram->programId, 3, "vertexTangent");
    lightingProgram->LinkProgram();
    

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

    
    // @@ To change an object's surface parameters (Kd, Ks, or alpha),
    // modify the following lines.
    central    = new Object(NULL, nullId);
    anim       = new Object(NULL, nullId);
    room       = new Object(RoomPolygons, roomId, brickColor, black, 1, roomTexture, BricksNormalT);//texture
    floor = new Object(FloorPolygons, floorId, floorColor, black, RoughSurface, floorTexture, FloorNormalT);//texture
    teapot = new Object(TeapotPolygons, teapotId, brassColor, lowSpecular, PolishedSurface, teapotTexture);//texture
    podium = new Object(BoxPolygons, boxId, glm::vec3(woodColor), lowSpecular, ModerateSmoothSurface, podiumTexture, PodiumNormalT);//texture
    //sky        = new Object(SpherePolygons, skyId, black, black,0, SkyTexture);
    sky        = new Object(SpherePolygons, skyId, black, black,0, HDRP_SKYTexture);
    objectRoot->add(sky, Scale(2000.0, 2000.0, 2000.0));
    ground = new Object(GroundPolygons, groundId, grassColor, black, RoughSurface, groundTexture);//texture
    //sea        = new Object(SeaPolygons, seaId, waterColor, lowSpecular, PolishedSurface, SkyTexture, RipplesNormalT);
    sea        = new Object(SeaPolygons, seaId, waterColor, lowSpecular, PolishedSurface, HDRP_SKYTexture, RipplesNormalT);
    leftFrame  = FramedPicture(Identity, lPicId, BoxPolygons, QuadPolygons, NULL);//No texture
    rightFrame = FramedPicture(Identity, rPicId, BoxPolygons, QuadPolygons,HouseTexture);//texture
    spheres    = SphereOfSpheres(SpherePolygons);


    CHECKERROR;

#ifdef REFL
    spheres->drawMe = true;
#else
    spheres->drawMe = false;
#endif


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

    CHECKERROR;

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
    
        //ImGui::SliderFloat("e",&e, 0.0f, 10.0f);


        ImGui::EndMainMenuBar(); }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::BuildTransforms()
{
    // @@ When you are ready to try interactive viewing, replace the
    // following hard coded values for WorldProj and WorldView with
    // transformation matrices calculated from variables such as spin,
    // tilt, tr, ry, front, and back.
    
    /*
    WorldProj[0][0]=  2.368;
    WorldProj[1][0]= -0.800;
    WorldProj[2][0]=  0.000;
    WorldProj[3][0]=  0.000;
    WorldProj[0][1]=  0.384;
    WorldProj[1][1]=  1.136;
    WorldProj[2][1]=  2.194;
    WorldProj[3][1]=  0.000;
    WorldProj[0][2]=  0.281;
    WorldProj[1][2]=  0.831;
    WorldProj[2][2]= -0.480;
    WorldProj[3][2]= 42.451;
    WorldProj[0][3]=  0.281;
    WorldProj[1][3]=  0.831;
    WorldProj[2][3]= -0.480;
    WorldProj[3][3]= 43.442;
    WorldView[3][0]= 0.0;
    WorldView[3][1]= 0.0;
    WorldView[3][2]= 0.0;
    */
    
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
    
    L_ProjectionMatrix = Perspective(Rx, Ry, front, back/5);//back -> 5000
    //std::cout << glm::to_string(L_ProjectionMatrix) << std::endl;
    //
    //Lighting Claculations
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
    

    CHECKERROR;
    // Calculate the light's position from lightSpin, lightTilt, lightDist
    lightPos = glm::vec3(lightDist*cos(lightSpin*rad)*sin(lightTilt*rad),
                         lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
                         lightDist*cos(lightTilt*rad));

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

    CHECKERROR;
    int loc, programId, ShadowPrgId;

    ////////////////////////////////////////////////////////////////////////////////
    // Shadow pass
    ////////////////////////////////////////////////////////////////////////////////
    //Create FBO
    //
    // Choose the lighting shader
    ShadowProgram->UseShader();
    ShadowPrgId = ShadowProgram->programId;
    //
    // Set the viewport, and clear the screen
    Shadow_Fbo->BindFBO();

    glViewport(0, 0, ShadowMap_Width, ShadowMap_Height);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    //glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);


    loc = glGetUniformLocation(ShadowPrgId, "L_ViewMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(L_ViewMatrix));
    loc = glGetUniformLocation(ShadowPrgId, "L_ProjectionMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(L_ProjectionMatrix));
    

    CHECKERROR;
    // Draw all objects (This recursively traverses the object hierarchy.)
    objectRoot->Draw(ShadowProgram, Identity);
    CHECKERROR;

    glDisable(GL_CULL_FACE);

    //For the DepthTest
    Shadow_Fbo->UnbindFBO();
    //
    // Turn off the shader
    ShadowProgram->UnuseShader();
    //
    ////////////////////////////////////////////////////////////////////////////////
    // End of Shadow pass
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
    Shadow_Fbo->BindTexture(2, programId, "shadowMap");
    //
    loc = glGetUniformLocation(programId, "SIGN");
    glUniform1f(loc,1.0);//upper map
    //
    loc = glGetUniformLocation(programId, "FLAG");
    glUniform1f(loc, true);//upper map

    CHECKERROR;
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
    CHECKERROR
    Shadow_Fbo->UnbindTexture(2);
    CHECKERROR
    Lower_Fbo->UnbindFBO();
    CHECKERROR
    // Turn off the shader
    ReflectionProgram->UnuseShader();
    CHECKERROR
    ////////////////////////////////////////////////////////////////////////////////
    // End of Reflections pass
    ////////////////////////////////////////////////////////////////////////////////

    
    //////////////////////////////////////////////////////////////////////////////////
    //// Lighting pass
    //////////////////////////////////////////////////////////////////////////////////
    teapot->drawMe = true;
        CHECKERROR
        // Choose the lighting shader
        lightingProgram->UseShader();
        CHECKERROR
        programId = lightingProgram->programId;
        CHECKERROR
        // Set the viewport, and clear the screen
        glViewport(0, 0, width, height);
        glClearColor(0.5, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //
        // @@ The scene specific parameters (uniform variables) used by
        // the shader are set here.  Object specific parameters are set in
        // the Draw procedure in object.cpp
        //
        CHECKERROR
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
        loc = glGetUniformLocation(programId, "Light");
        glUniform3fv(loc, 1, &(Light[0]));
        loc = glGetUniformLocation(programId, "Ambient");
        glUniform3fv(loc, 1, &(Ambient[0]));
        loc = glGetUniformLocation(programId, "ShadowMatrix");
        glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));

        /*
        loc = glGetUniformLocation(programId, "e");
        glUniform1f(loc, -1.0);//lower map
        */

        CHECKERROR
        //
        //
        //Upper Reflection Map
        //Lower Reflection Map
        CHECKERROR;
        Shadow_Fbo->BindTexture(2, programId, "shadowMap");
        Up_Fbo->BindTexture(3, programId, "UpMap");
        CHECKERROR;
        Lower_Fbo->BindTexture(4, programId, "LowerMap");
        HDRP_IrradianceTxt->BindTexture(5, programId, "IrradianceMap");
        HDRP_SKYTexture->BindTexture(6, programId, "SkyMapHDR");
        //
        CHECKERROR;
        //
        /*
        glActiveTexture(GL_TEXTURE2); // Activate texture unit 2
        glBindTexture(GL_TEXTURE_2D, Fbo->GetShadowMapTexture()); // Load texture into it
        loc = glGetUniformLocation(programId, "shadowMap");
        glUniform1i(loc, 2);
        */
         
        CHECKERROR;
        // Draw all objects (This recursively traverses the object hierarchy.)
        objectRoot->Draw(lightingProgram, Identity);

        Shadow_Fbo->UnbindTexture(2);
        //
        Up_Fbo->UnbindTexture(3);
        Lower_Fbo->UnbindTexture(4);
        HDRP_IrradianceTxt->UnbindTexture(5);
        HDRP_SKYTexture->UnbindTexture(6);
        //
        // Turn off the shader
        lightingProgram->UnuseShader();
        ////////////////////////////////////////////////////////////////////////////////
        // End of Lighting pass
        ////////////////////////////////////////////////////////////////////////////////
        
    PreviousTime = glfwGetTime();
}



