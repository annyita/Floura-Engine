#ifndef TEXTURE3D_CLASS_H
#define TEXTURE3D_CLASS_H

#include <glad/gl.h>

#include "Render/Shader/shaderClass.h"

class Texture3D{
public:
    
    GLuint ID; // texture ID
    GLuint64 handle; /// for bindless textures
    GLuint64 handleImage;
    int width = 0; int height = 0; int depth = 0;
    GLenum filtering = GL_LINEAR;
    
    void createTexture3D(const char* path, const char* texType, GLuint slot);
    void createImage3D(int w, int h, int d, const char* texType, GLuint slot, GLenum format);
    
    void Bind();
    void Unbind();
    void Delete();

    std::string path = "NULL";
    std::string type = "NULL";
    GLuint slot = 0;
    bool created = false;
    bool skipstbi = false;
    bool linearFilter = false;
    bool failed = false;
    bool enablePrint = true;
    
private:
    int atlasSize = 0;
    int sizeFromPath(std::string path);

};
#endif

//texture header is not the problem