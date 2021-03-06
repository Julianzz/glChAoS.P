////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018 Michele Morrone
//  All rights reserved.
//
//  mailto:me@michelemorrone.eu
//  mailto:brutpitt@gmail.com
//  
//  https://github.com/BrutPitt
//
//  https://michelemorrone.eu
//  https://BrutPitt.com
//
//  This software is distributed under the terms of the BSD 2-Clause license:
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright
//        notice, this list of conditions and the following disclaimer.
//      * Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//   
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "glslProgramObject.h"
#include "glslShaderObject.h"

#include "ParticlesUtils.h"
#include "palettes.h"



//-----------------------------------------------------------------------------
// Name: getRandomMinMax()
// Desc: Gets a random number between min/max boundaries
//-----------------------------------------------------------------------------
float getRandomMinMax( float fMin, float fMax )
{
    float fRandNum = (float)rand () / RAND_MAX;
    return fMin + (fMax - fMin) * fRandNum;
}

//-----------------------------------------------------------------------------
// Name: getRandomVector()
// Desc: Generates a random vector where X,Y, and Z components are between
//       -1.0 and 1.0
//-----------------------------------------------------------------------------
vec3 getRandomVector( void )
{
	vec3 vVector;

    // Pick a random Z between -1.0f and 1.0f.
    vVector.z = getRandomMinMax( -1.0f, 1.0f );

    // Get radius of this circle
    float radius = (float)sqrt(1 - vVector.z * vVector.z);

    // Pick a random point on a circle.
    float t = getRandomMinMax( -M_PI, M_PI );

    // Compute matching X and Y for our Z.
    vVector.x = (float)cosf(t) * radius;
    vVector.y = (float)sinf(t) * radius;

	return vVector;
}

//------------------------------------------------------------------------------
// Function     	  : EvalHermite
// Description	    :
//------------------------------------------------------------------------------
/**
* EvalHermite(float pA, float pB, float vA, float vB, float u)
* @brief Evaluates Hermite basis functions for the specified coefficients.
*/
inline float evalHermite(float pA, float pB, float vA, float vB, float u)
{
    float u2=(u*u), u3=u2*u;
    float B0 = 2*u3 - 3*u2 + 1;
    float B1 = -2*u3 + 3*u2;
    float B2 = u3 - 2*u2 + u;
    float B3 = u3 - u;
    return( B0*pA + B1*pB + B2*vA + B3*vB );
}


float* createGaussianMap(int N, int components)
{

    float *M = new float[components*(N+1)*(N+1)];
    float *B = new float[components*(N+1)*(N+1)];
    float X,Y,Y2,Dist;
    float Incr = 2.0f/N;
    int i=0;
    int j = 0;
    Y = -1.0f;
    //float mmax = 0;
    for (int y=0; y<N; y++, Y+=Incr)
    {
        Y2=Y*Y;
        X = -1.0f;
        for (int x=0; x<N; x++, X+=Incr, i+=components, j+=components)
        {
            Dist = (float)sqrtf(X*X+Y2);
            if (Dist>1) Dist=1;
            M[i+1] = M[i] = evalHermite(.7f,0.0,0.3,0.0,Dist);
            for(int k=0; k<components; k++)
                B[j+k] = M[i]; //(unsigned char)(M[i] * 255);
        }
    }
    delete [] M;
    return(B);
}

#define unsesto  .166666666666666666667
#define unterzo  .333333333333333333333
#define dueterzi .666666666666666666667


vec3 HLStoRGB( vec3 HLS)
{

    if(HLS.z==0.0) { return vec3(HLS.y,HLS.y,HLS.y); }

    if(HLS.x<0.0) HLS.x+=1.0;
    if(HLS.x>1.0) HLS.x-=1.0;

    //if(HLS.y>1) HLS.y = 1;
    //if(HLS.z>1) HLS.z = 1;

    float v2 = (HLS.y <= 0.5) ? HLS.y*(1.0+HLS.z) : (HLS.y+HLS.z) - HLS.y*HLS.z;
    float v1 = 2.0*HLS.y - v2;
    float v2_v1 = (v2-v1) * 6.0;

    if (HLS.x < unsesto )       return vec3(v2,
                                             v1 + v2_v1*HLS.x,
                                             v1 + v2_v1*(HLS.x-unterzo));
    else if (HLS.x < .5 )       return vec3(v1 + v2_v1*(unterzo-HLS.x),
                                             v2,
                                             v1 + v2_v1*(HLS.x-unterzo));
    else if (HLS.x < dueterzi ) return vec3(v1,
                                             v1 + v2_v1*(dueterzi-HLS.x),
                                             v2);
    else                        return vec3(v1,
                                             v1,
                                             v1 + v2_v1*(unterzo-HLS.x));

 }



void textureBaseClass::buildTex1D()
{

    if(texID!=-1) {
        glDeleteTextures(1,&texID);
        CHECK_GL_ERROR();
    }
#ifdef GLAPP_REQUIRE_OGL45
    glCreateTextures(GL_TEXTURE_1D, 1, &texID);
    glTextureParameteri(texID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(texID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texID, GL_TEXTURE_WRAP_S, GL_REPEAT );
#else
    glGenTextures(1, &texID);					// Generate OpenGL texture IDs
    glBindTexture(GL_TEXTURE_1D, texID);			// Bind Our Texture
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT );
#endif

    

}
/*
void textureBaseClass::buildTex2D()
{

    if(texID!=-1) {
        glDeleteTextures(1,&texID);
        CHECK_GL_ERROR();
    }

    glGenTextures(1, &texID);					// Generate OpenGL texture IDs

    glBindTexture(GL_TEXTURE_2D, texID);			// Bind Our Texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}
*/
void HLSTexture::buildTex(int size)
{


    buildTex1D();
    vec3 *buffer = new vec3[size];

    for(int i=0;i<size; i++) buffer[i] = HLStoRGB(vec3((float)i/(float)size,.5f,.99f));
#ifdef GLAPP_REQUIRE_OGL45
    glTextureStorage1D(texID, 1, GL_RGB32F, size);
    glTextureSubImage1D(texID, 0, 0, size, GL_RGB, GL_FLOAT, buffer);
#else    
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, size, 0, GL_RGB, GL_FLOAT, buffer);
#endif
    CHECK_GL_ERROR();

    texSize = size;

    delete[] buffer;

}


sigmaTextureClass::sigmaTextureClass()
{
    

}

sigmaTextureClass::~sigmaTextureClass()
{

}

void sigmaTextureClass::buildTex(int size, float sigma)
{


    buildTex1D();
    texSize = size;



#ifdef GLAPP_REQUIRE_OGL45
    glTextureStorage1D(texID, 1, GL_R32F, size);
#else
    glTexStorage1D(GL_TEXTURE_2D, 1, GL_R32F, size);
#endif

    rebuild(sigma);


    CHECK_GL_ERROR();



}

#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI

void sigmaTextureClass::rebuild(float sigma)
{
    const int size = (texSize>>1)-1;
	const float radius = 3.0*sigma-1.f;
    const float step = radius/float(size+1);

    float *buffer = new float[texSize];

    const float invSigma = 1.f/sigma;
    const float invSigmaSqx2 = .5 * invSigma * invSigma;          // 1.0 / (sigma^2 * 2.0)
    const float invSigmaxSqrt2PI = INV_SQRT_OF_2PI * invSigma;    // 1.0 / (sqrt(PI) * sigma)

    float f=-radius;
    for(int i=-size, j=0;i<=size; i++, j++, f+=step) 
        buffer[j] = exp( -(f*f) * invSigmaSqx2 ) * invSigmaxSqrt2PI;
 
#ifdef GLAPP_REQUIRE_OGL45
    glTextureSubImage1D(texID, 0, 0, texSize, GL_RED, GL_FLOAT, buffer);
#else    
    glTexSubImage1D(GL_TEXTURE_2D, 0, 0, texSize, GL_RED, GL_FLOAT, buffer);
#endif

    delete[] buffer;
}

HLSTexture::HLSTexture()
{
    

}

HLSTexture::~HLSTexture()
{

}

void RandomTexture::buildTex(int size)
{
    buildTex1D();

    vec3 *buffer = new vec3[size];

    for(int i=0;i<size; i++) buffer[i] = (getRandomVector() + vec3(1.0f)) * .5f;

#ifdef GLAPP_REQUIRE_OGL45
    glTextureStorage1D(texID, 1, GL_RGB32F, size);
    glTextureSubImage1D(texID, 0, 0, size, GL_RGB, GL_FLOAT, buffer);
#else
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, size, 0, GL_RGB, GL_FLOAT, buffer);
#endif
    CHECK_GL_ERROR();

    texSize = size;

    delete[] buffer;

}


RandomTexture::RandomTexture()
{
    srand( (unsigned)time( NULL ) );

    texIndex = 0;

}

RandomTexture::~RandomTexture()
{

}


void paletteTexClass::buildTex(unsigned char *buffer, int size)
{
    buildTex1D();
#ifdef GLAPP_REQUIRE_OGL45
    glTextureStorage1D(texID, 1, GL_RGB32F, size);
    glTextureSubImage1D(texID, 0, 0, size, GL_RGB, GL_UNSIGNED_BYTE, buffer);
#else
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, size, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
#endif
    
    CHECK_GL_ERROR();

    texSize = size;
}

void paletteTexClass::buildTex(float *buffer, int size)
{
    buildTex1D();
#ifdef GLAPP_REQUIRE_OGL45
    glTextureStorage1D(texID, 1, GL_RGB32F, size);
    glTextureSubImage1D(texID, 0, 0, size, GL_RGB, GL_FLOAT, buffer);
#else
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, size, 0, GL_RGB, GL_FLOAT, buffer);
#endif
    CHECK_GL_ERROR();

    texSize = size;
}



TextureView::TextureView(int x, int y, float r)
{
    texSize = ivec2(x, y);

    setReduction(r);

    onReshape(x, y);
   
}

void TextureView::SetOrtho()
{
/*
    glMatrixMode(GL_PROJECTION);
    //glPushMatrix();
    glLoadIdentity();
    glViewport(0,0,winSize.x, winSize.y);
    //glViewport(0,0,1024,1024);
    glOrtho(-1.0, 1.0, -1.0, 1.0, -2.0, 2.0);

    glMatrixMode(GL_MODELVIEW);
    //glPushMatrix();
    glLoadIdentity();
    gluLookAt (0.f, 0.f, 1.f,
               0.f, 0.f, 0.f,
               0.f, 1.f, 0.f);
*/
}


void TextureView::End(int w, int h)
{
}



void TextureView::onReshape(int w, int h)
{
    winSize = ivec2(w,h);   

    texInvSize = vec2(1.0/(float)w,1.0/(float)h) / reduction;

    wAspect = (w<h) ? vec2(1.0,(float)h/(float)w) : 
                      vec2((float)w/(float)h, 1.0);
   
}
