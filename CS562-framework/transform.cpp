////////////////////////////////////////////////////////////////////////
// A small library of 4x4 matrix operations needed for graphics
// transformations.  glm::mat4 is a 4x4 float matrix class with indexing
// and printing methods.  A small list or procedures are supplied to
// create Rotate, Scale, Translate, and Perspective matrices and to
// return the product of any two such.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include "math.h"
#include "transform.h"

float* Pntr(glm::mat4& M)
{
    return &(M[0][0]);
}

//@@ The following procedures should calculate and return 4x4
//transformation matrices instead of the identity.

// Return a rotation matrix around an axis (0:X, 1:Y, 2:Z) by an angle
// measured in degrees.  NOTE: Make sure to convert degrees to radians
// before using sin and cos.  HINT: radians = degrees*PI/180
const float pi = 3.14159f;
glm::mat4 Rotate(const int i, const float theta)
{
    glm::mat4 R = glm::mat4(1.0f);
    float CosAng = glm::cos(theta * pi / 180);
    float SinAng = glm::sin(theta * pi / 180);

    //theta is angle sent
    if (i == 0) //X Rotation
    {
        R[1][1] = CosAng;
        R[2][1] = -SinAng;
        R[1][2] = SinAng;
        R[2][2] = CosAng;
    }
    else if (i == 1)//Y Rotation
    {
        R[0][0] = CosAng;
        R[2][0] = SinAng;
        R[0][2] = -SinAng;
        R[2][2] = CosAng;
    }
    else if (i == 2)//Z Rotation
    {
        R[0][0] = CosAng;
        R[1][0] = -SinAng;
        R[0][1] = SinAng;
        R[1][1] = CosAng;
    }

    return R;
}

// Return a scale matrix
glm::mat4 Scale(const float x, const float y, const float z)
{
    glm::mat4 S = glm::mat4(1.0f);
    S[0][0] = x;
    S[1][1] = y;
    S[2][2] = z;
    return S;
}

// Return a translation matrix
glm::mat4 Translate(const float x, const float y, const float z)
{
    glm::mat4 T = glm::mat4(1.0);
    T[3][0] = x;
    T[3][1] = y;
    T[3][2] = z;

    return T;
}

// Returns a perspective projection matrix
glm::mat4 Perspective(const float rx, const float ry,
             const float front, const float back)
{
    glm::mat4 P(0.0);
    P[0][0] = 1 / rx;
    P[1][1] = 1 / ry;
    P[2][2] = -((back + front) / (back - front));
    P[3][2] = -((2 * (front * back) / (back - front)));
    P[2][3] = -1;

    return P;
}


//
//re-defined it  
glm::mat4 LookAt(const glm::vec3 Eye, const glm::vec3 Center, const glm::vec3 Up)
{
    //Camera Forward
    glm::vec3 V = glm::normalize(Center - Eye);
    //Right Vector
    glm::vec3 A = glm::normalize(glm::cross(V, Up));
    //Camera Up
    glm::vec3 B = glm::cross(A,V);
    
    //glm::mat4 T = Translate(-Eye.x, -Eye.y, -Eye.z);//Translating the Eye to Origin
    
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(-Eye.x, -Eye.y, -Eye.z));  // Translate Eye to Origin

    /*//row major
    glm::mat4 Rotation = glm::mat4( A.x, A.y, A.z, 0.0f,
                                    B.x, B.y, B.z, 0.0f,
                                   -V.x, -V.y, -V.z, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f);
    */

    //Column Major
    glm::mat4 Rotation = glm::mat4( A.x, B.x, -V.x, 0.0f,
                                    A.y, B.y, -V.y, 0.0f,
                                    A.z, B.z, -V.z, 0.0f,
                                    0.0f, 0.0f, 0.0f, 1.0f);


    glm::mat4 LookAt = Rotation * T;

    return LookAt;

}





