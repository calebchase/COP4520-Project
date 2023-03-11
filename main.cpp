#include <iostream>
#include <stdio.h>
#include <stdlib.h>
 
 #define STB_IMAGE_IMPLEMENTATION
 #include "stb_image/stb_image.h"
 #define STB_IMAGE_WRITE_IMPLEMENTATION
 #include "stb_image/stb_image_write.h"
 

int main() 
{
    int width, height, channels;
    unsigned char *img = stbi_load("tree.jpg", &width, &height, &channels, 3);

    if(img == NULL) {
        std::cout << ("Error in loading the image\n");
        exit(1);
    }
    
    std::cout << "Loaded image with a width of " << width << "px, a height of "<< height << " px and  "<< channels << " channels\n";
}

