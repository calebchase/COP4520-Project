#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

using namespace std;

struct PixelYCbCr
{
    int Y;
    int Cb;
    int Cr;
};

struct PixelRGB
{
    int R;
    int G;
    int B;
};

PixelYCbCr toYCbCr(int R, int G, int B)
{
    PixelYCbCr outColor = PixelYCbCr();

    outColor.Y = R * 0.229 + G * 0.587 + B * 0.144;
    outColor.Cb = R * -0.168935 + G * -0.331665 + B * 0.50059 + 128;
    outColor.Cr = R * 0.499813 + G * -0.418531 + B * -.081282 + 128;

    return outColor;
}

vector<vector<PixelYCbCr>> *CreateYCbCrArray(unsigned char *img, vector<vector<PixelYCbCr>> *outVector, int channels)
{
    int dataIndex = 0;
    for (int i = 0; i < outVector->size(); i++)
    {
        for (int j = 0; j < (*outVector)[i].size(); j++)
        {
            (*outVector)[i][j] = toYCbCr(img[dataIndex], img[dataIndex + 1], img[dataIndex + 2]);
            dataIndex += 3;
        }
    }

    return outVector;
}

void WriteYCbCrArray(int width, int height, int channels, vector<vector<PixelYCbCr>> *outVector)
{
    unsigned char *outData = new unsigned char[width * height * channels];

    int dataIndex = 0;
    for (int i = 0; i < outVector->size(); i++)
    {
        for (int j = 0; j < (*outVector)[i].size(); j++)
        {

            outData[dataIndex++] = (unsigned char)(*outVector)[i][j].Y;
            outData[dataIndex++] = (unsigned char)(*outVector)[i][j].Cb;
            outData[dataIndex++] = (unsigned char)(*outVector)[i][j].Cr;
        }
    }

    stbi_write_bmp("output.bmp", width, height, 3, outData);
}

int main()
{
    int width, height, channels;
    unsigned char *img = stbi_load("lenna.png", &width, &height, &channels, 3);

    if (img == NULL)
    {
        std::cout << ("Error in loading the image\n");
        exit(1);
    }

    std::cout << "Loaded image with a width of " << width << "px, a height of " << height << " px and  " << channels << " channels\n";
    vector<vector<PixelYCbCr>> outVector(height, vector<PixelYCbCr>(width));

    CreateYCbCrArray(img, &outVector, 3);
    WriteYCbCrArray(width, height, 3, &outVector);
}
