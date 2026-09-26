#include "imageResize.h"
#include <cstdlib>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"

void FlouraImageResize::resize2D(unsigned char* &ibytes, int &iwidthImg, int &iheightImg, int &inumColCh, unsigned char* &obytes, int &owidthImg, int &oheightImg){
    //obytes = (unsigned char*)malloc(owidthImg * oheightImg * inumColCh);
    int success = stbir_resize_uint8(ibytes ,iwidthImg, iheightImg, 0,
    obytes, owidthImg, oheightImg, 0, inumColCh);
    
}
