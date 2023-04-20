#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "huffman.hpp"
#include "CompressorThread.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

using namespace std;

/*
struct PixelYCbCr
{
	int Y;
	int Cb;
	int Cr;
};
*/

struct PixelRGB
{
	int R;
	int G;
	int B;
};

static const int quantizationMatrix[8][8] =
{
	{ 16, 11, 10, 16, 24, 40, 51, 61 },
	{ 12, 12, 14, 19, 26, 58, 60, 55 },
	{ 14, 13, 16, 24, 40, 57, 69, 56 },
	{ 14, 17, 22, 29, 51, 87, 80, 62 },
	{ 18, 22, 37, 56, 68, 109, 103, 77 },
	{ 24, 35, 55, 64, 81, 104, 113, 92 },
	{ 49, 64, 78, 87, 103, 121, 120, 101 },
	{ 72, 92, 95, 98, 112, 100, 103, 99 }
};

static const int chrominanceQuantizationMatrix[8][8] =
{
    { 17, 18, 24, 47, 99, 99, 99, 99 },
    { 18, 21, 26, 66, 99, 99, 99, 99 },
    { 24, 26, 56, 99, 99, 99, 99, 99 },
    { 47, 66, 99, 99, 99, 99, 99, 99 },
    { 99, 99, 99, 99, 99, 99, 99, 99 },
    { 99, 99, 99, 99, 99, 99, 99, 99 },
    { 99, 99, 99, 99, 99, 99, 99, 99 },
    { 99, 99, 99, 99, 99, 99, 99, 99 }
};


static const int zigzag[64] =
{
	0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63
};

PixelYCbCr toYCbCr(int R, int G, int B)
{
	PixelYCbCr outColor = PixelYCbCr();

	outColor.Y = R * 0.229 + G * 0.587 + B * 0.144;
	outColor.Cb = R * -0.168935 + G * -0.331665 + B * 0.50059 + 128;
	outColor.Cr = R * 0.499813 + G * -0.418531 + B * -.081282 + 128;

	return outColor;
}

void CreateYCbCrArray(unsigned char* img, vector<vector<PixelYCbCr>>& outVector, int width, int height, int channels)
{
	int dataIndex = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			outVector[i][j] = toYCbCr(img[dataIndex], img[dataIndex + 1], img[dataIndex + 2]);
			dataIndex += 3;
		}
	}

}

void WriteYCbCrArray(int width, int height, int channels, vector<vector<PixelYCbCr>>& outVector, const char* name)
{
	unsigned char* outData = new unsigned char[width * height * channels];

	int dataIndex = 0;
	cout << outVector.size();
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{

			outData[dataIndex++] = (unsigned char)(outVector)[i][j].Y;
			outData[dataIndex++] = (unsigned char)(outVector)[i][j].Cb;
			outData[dataIndex++] = (unsigned char)(outVector)[i][j].Cr;
		}
	}

	stbi_write_bmp(name, width, height, 3, outData);
	cout << "Image written" << endl;
}

float DCTCosVal(int firstIndex, int secondIndex)
{
	return std::cos((3.14159262 / (8.0)) * (firstIndex + 0.5) * secondIndex);
}

int getPixelVal(PixelYCbCr pixel, int type)
{
	if (type == 0)
		return pixel.Y;
	if (type == 1)
		return pixel.Cb;
	if (type == 2)
		return pixel.Cr;
	return -1;
}

// Might have bugs
void DCT(vector<vector<PixelYCbCr>>& pixelArray, int rowIndex, int colIndex, int type)
{
	float ci = 0;
	float cj = 0;
	float sum = 0;
	float dctTemp[8][8];

	pixelArray[0 + 0][0 + colIndex].Y = 255;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			ci = i == 0 ? 1 / std::sqrt(8.0) : sqrt(2.0) / sqrt(8.0);
			cj = j == 0 ? 1 / std::sqrt(8.0) : sqrt(2.0) / sqrt(8.0);

			sum = 0;
			for (int k = 0; k < 8; k++)
			{
				for (int l = 0; l < 8; l++)
				{
					sum += getPixelVal(pixelArray[i + rowIndex][j + colIndex], type) * 
						cos((2 * k  + 1) * i * 3.14159 / 16.0) *
						cos((2 * l + 1) * j * 3.14159 / 16.0);
				}
			}
			dctTemp[i][j] = ci * cj * sum;
		}
	}

	return;
}

// Input: array of DCT coefficients
// Output: quantized values
void Quantize(vector<vector<PixelYCbCr>>& pixelArray, int y, int x)
{
	for (int i = y; i < y + 8; i++)
	{
		for (int j = x; j < x + 8; j++)
		{
			pixelArray[i][j].Y = round((float)pixelArray[i][j].Y / quantizationMatrix[i][j]);
			pixelArray[i][j].Cb = round((float)pixelArray[i][j].Cb / chrominanceQuantizationMatrix[i][j]);
			pixelArray[i][j].Cr = round((float)pixelArray[i][j].Cr / chrominanceQuantizationMatrix[i][j]);
		}
	}
}

void Quantize(vector<vector<int>>& pixelArray, int rowIndex, int colIndex)
{
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			pixelArray[i][j] = round((float)pixelArray[i][j] / quantizationMatrix[i][j]);
		}
	}
}

// flattens a 2D array into a 1D array with same amount of elements
vector<int> flatten(vector<vector<int>>& pixelArray)
{
	vector<int> result;

	for (const auto& v : pixelArray)
	{
		result.insert(result.end(), v.begin(), v.end());
	}

	return result;
}

// collects data in a zig zag motion
vector<int> zigzagEncoding(vector<int> pixelArray)
{
	vector<int> result(64, 0);

	for (int i = 0; i < 64; i++)
	{
		result[i] = pixelArray[zigzag[i]];
	}

	return result;
}

vector<int> runDeltaEncoding(vector<int> arr)
{
	vector<int> result(arr.size(), 0);
	int DC = arr[0];

	for (int i = 1; i < 64; i++)
	{
		result[i] = arr[i] - DC;
	}

	return result;
}

vector<pair<int, int>> runLengthEncoding(vector<int> arr)
{
	vector<pair<int, int>> result;

	int val = arr[0];
	int count = 1;
	for (int i = 1; i < arr.size(); i++)
	{
		if(val == arr[i])
		{
			count++;
		}
		else
		{
			result.push_back(pair(val, count));
			count = 1;
			val = arr[i];
		}
	}

	result.push_back(pair(val, count));

	return result;
}

void testHuffmanEncoding(vector<int>& orig, vector<int>& decoded)
{
	assert(orig.size() == decoded.size());

	for(int i = 0; i < orig.size(); i++)
	{
		assert(orig[i] == decoded[i]);
	}
}

void testThreadedYCbCr(vector<vector<PixelYCbCr>>& r1, vector<vector<PixelYCbCr>>& r2)
{
	//assert(r1.size() == r2.size());

	for(int i = 0; i < r2.size(); i++)
	{
		for(int j = 0; j < r2[i].size(); j++)
		{
			if((r1[i][j].Y != r2[i][j].Y) || (r1[i][j].Cb != r2[i][j].Cb) || (r1[i][j].Cr != r2[i][j].Cr))
			{
				cout << "not equal" << endl;
				break;
			}
			
		}
	}
}

void quantizeAndEncode(vector<vector<PixelYCbCr>>& vec, int startX, int startY)
{
	Quantize(vec, startY, startX);
}


void compressToJPG(unsigned char* img, const int width, const int height, int channels)
{

	int paddingX = 8 - (width % 8);
	int paddingY = 8 - (height % 8);

	paddingX = paddingX == 8 ? 0 : paddingX;
	paddingY = paddingY == 8 ? 0 : paddingY;

	int paddedWidth = paddingX + width;
	int paddedHeight = paddingY + height;

	vector<vector<PixelYCbCr>> pixelArray((paddedHeight), vector<PixelYCbCr>(paddedWidth));
	CreateYCbCrArray(img, pixelArray, width, height, 3);

	for(int y = 0;  y < paddedWidth; y += 8)
	{
		for(int x = 0; x < paddedHeight; x += 8)
		{
			Quantize(pixelArray, y, x);
		}
	}
}

int main()
{
	int NUM_THREADS = 4;
	thread* threads = new thread[NUM_THREADS];

	int width, height, channels;
	unsigned char* img = stbi_load("tree.jpg", &width, &height, &channels, 3);

	if (img == NULL)
	{
		std::cout << ("Error in loading the image\n");
		exit(1);
	}

	std::cout << "Loaded image with a width of " << width << "px, a height of " << height << " px and  " << channels << " channels\n";

	// Calculate padding around border of image
	int paddingX = 8 - (width % 8);
	int paddingY = 8 - (height % 8);

	paddingX = paddingX == 8 ? 0 : paddingX;
	paddingY = paddingY == 8 ? 0 : paddingY;

	vector<vector<PixelYCbCr>> pixelArray((height + paddingY), vector<PixelYCbCr>(width + paddingX));
	vector<vector<PixelYCbCr>> pixelArray2((height), vector<PixelYCbCr>(width));

	CreateYCbCrArray(img, pixelArray2, width, height, channels);
	//DCT(pixelArray2, 0, 0, 0);

	atomic_int col_index(0);
	atomic_int row_index(0);
	atomic_bool done(false);

	auto startTime = std::chrono::high_resolution_clock::now();

	for(int i = 0; i < NUM_THREADS; i++)
	{
		threads[i] = thread(CompressorThread(i, img, (width), (height), channels), ref(row_index), ref(col_index), ref(done), ref(pixelArray)); 
	}

	for(int i = 0; i < NUM_THREADS; i++)
	{
		threads[i].join();
	}

	while(!done.load()) {}

	auto endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> runTime = endTime - startTime;
	cout << "Time for multithreaded: " << runTime.count() << endl;

	string name = "SingleThread.bmp";
	WriteYCbCrArray(width, height, 3, pixelArray2, name.c_str());
	
	name = "multiThread.bmp";
	WriteYCbCrArray(width, height, 3, pixelArray, name.c_str());

	vector<vector<int>> dctCoefficients = {
		{139, -39,  44, -25,   8, -24,   9,  -5},
		{  8, -21, -16,  13, -16,  -3, -10,   4},
		{  4,   4,   4, -11, -10,  -3,  -3,   4},
		{ -4,   4,   4,  -5,  -2,   5,   5,  -5},
		{ -6,  -6,  -1,  -1,   0,   1,  -1,   1},
		{  0,  -1,   0,   0,  -1,   0,   0,   1},
		{ -1,   0,  -1,   1,   0,   0,   0,   0},
		{ -1,  -1,   1,   0,   0,  -1,   0,   0}
	};
	
	
	Quantize(dctCoefficients, 0, 0);
	vector<int> flattened = flatten(dctCoefficients);
	vector<int> zigzag = zigzagEncoding(flattened);
	vector<pair<int, int>> rle = runLengthEncoding(zigzag);

	HuffmanNode* tree = buildTree(rle);
	map<int, string> table = buildTable(tree);
	string str = encode(zigzag, table);

	vector<int> decodeTest = decode(str, tree);

	testHuffmanEncoding(zigzag, decodeTest);
	testThreadedYCbCr(pixelArray, pixelArray2);


	return 0;
}