#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>

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

float DCTCosVal(int firstIndex, int secondIndex)
{
    return std::cos((3.14159262 / (8.0)) * (firstIndex + 0.5) * secondIndex);
}

int getPixelVal(PixelYCbCr *pixel, int type)
{
    if (type == 0)
        return pixel->Y;
    if (type == 1)
        return pixel->Cb;
    if (type == 2)
        return pixel->Cr;
    return -1;
}

// Might have bugs
void DCT(vector<vector<PixelYCbCr>> *pixelArray, int rowIndex, int colIndex, int type)
{
    float ci = 0;
    float cj = 0;
    float sum = 0;
    float dctTemp[8][8];

    (*pixelArray)[0 + 0][0 + colIndex].Y = 255;
    cout << (*pixelArray)[0][0].Y;

    // used for testing
    // for (int i = 0; i < 8; i++)
    // {
    //     for (int j = 0; j < 8; j++)
    //     {
    //         (*pixelArray)[i][j].Y = 255;
    //     }
    // }

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            ci = i == 0 ? 1 / std::sqrt(8.0) : std::sqrt(2) / std::sqrt(8);
            cj = j == 0 ? 1 / std::sqrt(8.0) : std::sqrt(2) / std::sqrt(8);

            sum = 0;
            for (int k = 0; k < 8; k++)
            {
                for (int l = 0; l < 8; l++)
                {
                    cout << getPixelVal(&((*pixelArray)[i + rowIndex][j + colIndex]), type);
                    sum += getPixelVal(&((*pixelArray)[i + rowIndex][j + colIndex]), type) * DCTCosVal(k, i) * DCTCosVal(l, j);
                }
                cout << "\n";
            }
            dctTemp[i][j] = ci * cj * sum;
        }
    }

    // used for testing
    // for (int i = 0; i < 8; i++)
    // {
    //     for (int j = 0; j < 8; j++)
    //     {
    //         cout << (float)dctTemp[i][j] << " ";
    //     }
    //     cout << "\n";
    // }
}

int main()
{
    int width, height, channels;
    unsigned char *img = stbi_load("tree.jpg", &width, &height, &channels, 3);

    if (img == NULL)
    {
        std::cout << ("Error in loading the image\n");
        exit(1);
    }

    std::cout << "Loaded image with a width of " << width << "px, a height of " << height << " px and  " << channels << " channels\n";
    vector<vector<PixelYCbCr>> pixelArray(height, vector<PixelYCbCr>(width));

    CreateYCbCrArray(img, &pixelArray, 3);
    DCT(&pixelArray, 0, 0, 0);

    // WriteYCbCrArray(width, height, 3, &pixelArray);
}
