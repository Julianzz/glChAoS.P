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
#pragma once

#include <iomanip>
#include <chrono>
#include <vector>
#include "glslProgramObject.h"
#include "glslShaderObject.h"
#include "appDefines.h"

#define COMPONENTS_PER_ATTRIBUTE 4

//#define GLAPP_REQUIRE_OGL45
//#define PRINT_TIMING

// Create three query objects
// Start the first query

class vertexBufferBaseClass 
{
public:
    vertexBufferBaseClass(GLenum primitive, int numVertex, int attributesPerVertex) : 
        attributesPerVertex(attributesPerVertex), primitive(primitive), nVtxStepBuffer(numVertex)
    {
#ifdef GLAPP_REQUIRE_OGL45 
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1,&vbo);
#else
        glGenBuffers(1,&vbo);
#endif
        bytesPerVertex = attributesPerVertex * COMPONENTS_PER_ATTRIBUTE * sizeof(float); 
        uploadedVtx = 0;
    }

    ~vertexBufferBaseClass() 
    {
#ifdef GLAPP_REQUIRE_OGL45 
        glDeleteVertexArrays(1, &vao);
#endif
        glDeleteBuffers(1,&vbo);
        delete [] vtxBuffer;
    }

    GLfloat* getBuffer()  { return vtxBuffer; }
    int      getBytesPerVertex() { return bytesPerVertex; }
    int      getNumComponents()  { return COMPONENTS_PER_ATTRIBUTE * attributesPerVertex; }
    int      getAttribPerVtx()  { return attributesPerVertex; }
    GLuint64 getVertexUploaded() { return uploadedVtx; }
    GLuint64 *getPtrVertexUploaded() { return &uploadedVtx; }
    void     incVertexCount() { uploadedVtx++;  }
    void     resetVertexCount()  { uploadedVtx = 0; }
    GLuint   getVBO()            { return vbo; };
    GLenum   getPrimitive() { return primitive; }

    void ActivateClientStates()  {
#if !defined(GLAPP_REQUIRE_OGL45)
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        for(int i =0; i<attributesPerVertex; i++) {
            glVertexAttribPointer(i, COMPONENTS_PER_ATTRIBUTE, GL_FLOAT, GL_FALSE,bytesPerVertex, (GLvoid *) (i*COMPONENTS_PER_ATTRIBUTE*sizeof(float))); 
            glEnableVertexAttribArray(i);
        } 
#endif
    }

    void DeactivateClientStates() {       
#if !defined(GLAPP_REQUIRE_OGL45)
        for(int i = 0; i<attributesPerVertex; i++) glDisableVertexAttribArray(i);
#endif
    }

    virtual void initBufferStorage(GLsizeiptr numElements) = 0;
    virtual void buildVertexAttrib() {
#ifdef GLAPP_REQUIRE_OGL45
        const int locID = 0;
        glVertexArrayAttribBinding(vao,locID, 0);
        glVertexArrayAttribFormat(vao, locID, COMPONENTS_PER_ATTRIBUTE, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, locID);        

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, bytesPerVertex);
#else
        ActivateClientStates();
        glBindBuffer(GL_ARRAY_BUFFER,0);
#endif
        CHECK_GL_ERROR();
    }


    virtual bool uploadSubBuffer(GLuint nVtx, GLuint szCircularBuff) 
    {
        const GLuint offset = uploadedVtx % szCircularBuff;
        const GLuint offByte = offset * bytesPerVertex;

        bool retVal = (offset+nVtx >= szCircularBuff) ? true : false;

        if(offset+nVtx > szCircularBuff) {            
            const GLuint szPart1 = (szCircularBuff - offset) * bytesPerVertex;
            const GLuint szPart2 = ((offset+nVtx) - szCircularBuff) * bytesPerVertex;
           
            //cout << szPart1 << " - " << szPart2 << endl;
#ifdef GLAPP_REQUIRE_OGL45
            glNamedBufferSubData(vbo, offByte, szPart1, vtxBuffer);
            glNamedBufferSubData(vbo, 0      , szPart2, (GLubyte *) vtxBuffer + szPart1); 
        } else {
            //cout << offByte << " - " << nVtx << endl;
            //glBindBuffer(GL_ARRAY_BUFFER,vbo);
            //glBufferSubData(GL_ARRAY_BUFFER, offByte, nVtx * bytesPerVertex, vtxBuffer); 

            glNamedBufferSubData(vbo, offByte, nVtx * bytesPerVertex, vtxBuffer); 
        }
#else
            glBindBuffer(GL_ARRAY_BUFFER,vbo);
            glBufferSubData(GL_ARRAY_BUFFER, offByte, szPart1, vtxBuffer);
            glBufferSubData(GL_ARRAY_BUFFER, 0      , szPart2, (GLubyte *) vtxBuffer + szPart1); 
        } else {
            glBindBuffer(GL_ARRAY_BUFFER,vbo);
            glBufferSubData(GL_ARRAY_BUFFER, offByte, nVtx * bytesPerVertex, vtxBuffer); 
        }
        glBindBuffer(GL_ARRAY_BUFFER,0);
#endif
        uploadedVtx+=nVtx;

        return retVal;
    }

    void draw(GLuint maxSize) {
#ifdef GLAPP_REQUIRE_OGL45
        glBindVertexArray(vao);
        glDrawArrays(primitive,0, uploadedVtx<maxSize ? uploadedVtx : maxSize);
#else
        ActivateClientStates();
        glDrawArrays(primitive,0,uploadedVtx<maxSize ? uploadedVtx : maxSize);
        DeactivateClientStates();
        CHECK_GL_ERROR();
#endif
    }

//  Feedback functions
////////////////////////////////////////////////////////////////////////////
    void BindToFeedback(int index)
    {
        glTransformFeedbackBufferRange(0,index,vbo,0,uploadedVtx*bytesPerVertex);
    }
    void uploadData(int numVtx) 
    {
        int bufferSize = numVtx * bytesPerVertex;
        uploadedVtx = numVtx;
   
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,bufferSize,&vtxBuffer[0],GL_STATIC_DRAW);        
        //glBufferSubData(GL_ARRAY_BUFFER,0,bufferSize,&vtxBuffer[0]);        
        //glInvalidateBufferData(vbo);
        glBindBuffer(GL_ARRAY_BUFFER,0);
    }
    void drawRange(GLuint start, GLuint size) 
    {
        ActivateClientStates();
        glDrawArrays(primitive,start,size);
        DeactivateClientStates();
    }

protected:
     GLfloat *vtxBuffer = nullptr;
    int attributesPerVertex;
    GLuint vbo,vao;
    GLuint nVtxStepBuffer;
    GLuint bytesPerVertex;           //Total bytes per Vertex: all attributes!
    //GLuint64 uploadedDataSize;
    GLuint64 uploadedVtx;
    GLenum primitive;
};

class mappedVertexBuffer : public vertexBufferBaseClass
{
public:

    mappedVertexBuffer(GLenum primitive, int numVertex, int attributesPerVertex) : 
        vertexBufferBaseClass(primitive, numVertex, attributesPerVertex) { }

    ~mappedVertexBuffer() {
#ifdef GLAPP_REQUIRE_OGL45
        glUnmapNamedBuffer(vbo);
#else
        //glBindBuffer(GL_ARRAY_BUFFER,vbo);        
        //glUnmapBuffer(GL_ARRAY_BUFFER);
#endif
    }

    void initBufferStorage(GLsizeiptr nVtx) {
        GLsizeiptr storageSize = nVtx * bytesPerVertex;

#ifdef GLAPP_REQUIRE_OGL45
        glNamedBufferStorage(vbo, storageSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT); // ); //  
        vtxBuffer = (GLfloat *) glMapNamedBufferRange(vbo, 0, storageSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);   //
#else
        //glBindBuffer(GL_ARRAY_BUFFER,vbo); 
        //glBufferData(GL_ARRAY_BUFFER, storageSize, nullptr, GL_DYNAMIC_DRAW ); // ); //
        //vtxBuffer = (GLfloat *) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE  );   //| GL_MAP_COHERENT_BIT
#endif
        buildVertexAttrib();
    }


};

class vertexBuffer : public vertexBufferBaseClass 
{
public:
    vertexBuffer(GLenum primitive, int numVertex, int attributesPerVertex) : 
        vertexBufferBaseClass(primitive, numVertex, attributesPerVertex) { }

    void initBufferStorage(GLsizeiptr nVtx) {
        GLsizeiptr storageSize = nVtx * bytesPerVertex;

#ifdef GLAPP_REQUIRE_OGL45
        vtxBuffer = new GLfloat[nVtxStepBuffer * attributesPerVertex * COMPONENTS_PER_ATTRIBUTE];
        glNamedBufferStorage(vbo, storageSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
        //glNamedBufferData(vbo, storageSize,nullptr,GL_DYNAMIC_DRAW);
#else
        vtxBuffer = new GLfloat[nVtxStepBuffer * attributesPerVertex * COMPONENTS_PER_ATTRIBUTE];
        glBindBuffer(GL_ARRAY_BUFFER,vbo);        
        glBufferData(GL_ARRAY_BUFFER,storageSize,nullptr,GL_DYNAMIC_DRAW);
#endif

        buildVertexAttrib();
    }
};


class transformFeedbackInterleaved {
  private:
    GLuint query;
    vertexBuffer *vbo;
    static bool FeedbackActive;
    bool bDiscard;

  public:
    transformFeedbackInterleaved(vertexBuffer *vbo)
    {
      this->vbo = vbo;
      glGenQueries(1,&query);
      bDiscard = true;
    }

    void Begin() {
        if (FeedbackActive) return;

        FeedbackActive = true;
        vbo->BindToFeedback(0);

        glBeginTransformFeedback(vbo->getPrimitive());

        if (bDiscard) glEnable(GL_RASTERIZER_DISCARD);

        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
    }

    int End() {
        int iPrimitivesWritten;
        if(!FeedbackActive)  return -1;
        FeedbackActive = false;

        if(bDiscard)  glDisable(GL_RASTERIZER_DISCARD);
        glEndTransformFeedback();

        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
        glGetQueryObjectiv(query,GL_QUERY_RESULT,&iPrimitivesWritten);
  
        return iPrimitivesWritten;

    }
    void SetDiscard(bool value){ bDiscard = true; }
};

#ifdef PRINT_TIMING
        //glBeginQuery(GL_TIME_ELAPSED, queries[0]);
        cout <<  "n.vtx:"<< std::setfill( ' ' ) << std::setw(7) << nVtx << " - ";
        auto before = std::chrono::system_clock::now();
#endif
#ifdef PRINT_TIMING
        //glEndQuery(GL_TIME_ELAPSED);
        auto now = std::chrono::system_clock::now();
        double diff_ms = std::chrono::duration <double, std::nano> (now - before).count();
        cout << std::setw(3) << std::fixed << std::setprecision(3) << std::setfill( '0' ) << "buff ms: " << float(diff_ms)/1000000.f;
#endif


#ifdef NON_MI_SERVE
    GLfloat *mapBuffer(GLuint nVtx) {
        uploadedVtx = 0;
        return (GLfloat *) glMapNamedBufferRange(vbo, 0, nVtx * bytesPerVertex, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    }

    void unmapBuffer(GLuint nVtx) {
        glUnmapNamedBuffer(vbo);
        uploadedVtx+=nVtx;
    }



    GLfloat* mapBuffer(GLuint nVtx, GLuint szCircularBuff)
    {
        //int bufferSize = nVtx * bytesPerVertex;
        //GLuint bytesCircularBuffer = szCircularBuff * bytesPerVertex;
        const GLuint offset = uploadedVtx % szCircularBuff;


        return (GLfloat *) ((GLbyte *) vtxBuffer + offset * bytesPerVertex); //glMapNamedBufferRange(vbo, offset, bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT  | GL_MAP_COHERENT_BIT);
    }


    void viewElapsed() {
#ifdef PRINT_TIMING
        glGetQueryObjectuiv(queries[0], GL_QUERY_RESULT, &buffFill);
        glGetQueryObjectuiv(queries[1], GL_QUERY_RESULT, &drawArray);
        
        cout << std::fixed << std::setprecision(3) << std::setfill( '0' ) << "Buff: " << float(buffFill)/1000000.f << " - Array: " << float(drawArray)/1000000.f << endl;
#endif
    }
        

    void drawVtx() 
    {
        ActivateClientStates();
        glDrawArrays(primitive,0,numVertex);
        DeactivateClientStates();

    }


#endif
