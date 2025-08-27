#include"Project2.h"
#include <cmath>
#include <numeric>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

Project2::Project2( ) {

}

Project2::Project2(int BpValue) : weights(101, 0.0f){

    Flag = false;
    bindpoint = BpValue;
    
    blurWidth = 10;
}


void Project2::CreateUniformBuffer( ) {

    Flag = true;
    glGenBuffers(1, &bufferId);

    UpdateWeights( );

    //
    //glBindBuffer(GL_UNIFORM_BUFFER, blockID)
    //    glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, bufferID)
    //    glBufferData(GL_UNIFORM_BUFFER, <#bytes>, <data pointer>, GL_STATIC_DRAW)
    //
}

//
//change the logic
void Project2::UpdateWeights( ) {
    
    int ReqWidth = (2 * blurWidth) + 1;// 2*1 + 1 => 3
    double s = blurWidth / 2.0;

    for (int i = -blurWidth; i <= blurWidth; ++i) {
        weights[i + blurWidth] = std::exp((- 0.5) * std::pow(i / s, 2));
    }

    double sum = 0.0;
    for (int i = -blurWidth; i <= blurWidth; ++i) {
        sum += weights[i + blurWidth];
    }

    for (int i = -blurWidth; i <= blurWidth; ++i) {

        weights[i + blurWidth] /= sum;
        
    }


    //std::cout <<"WEIGHTS"<< std::endl;
    //for (int i = 0; i < (2 * (blurWidth)+1); i++) {
    //    std::cout << weights[i] << std::endl;
    //}
  
    if (Flag == true) {

        glBindBuffer(GL_UNIFORM_BUFFER, bufferId);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, bufferId);
        glBufferData(GL_UNIFORM_BUFFER, weights.size() * sizeof(float), weights.data(), GL_STATIC_DRAW);
        //std::cout<< "SIZEOF --> " << (weights.size() * sizeof(float)) << std::endl;
    }
}


void Project2::IncrementKernal( ) {
    blurWidth++;
    UpdateWeights();
}

void Project2::DecrementKernal( ) {
    blurWidth--;
    UpdateWeights();
}

int  Project2::KernalValue() {

    return blurWidth;
}







