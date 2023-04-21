#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <atomic>
#include <algorithm>
using namespace std;

const float inverseSQR8 = 0.35355;
const float sqr2oversqr8 = 0.5;
const float piOver16 = 0.19635;

class CompressorThread
{
private:
	int id;
	unsigned char *img;
	int width;
	int height;
	int channels;

	int startRow;
	int startCol;

public:
	CompressorThread(int id, unsigned char *img, int width, int height, int channels)
	{
		this->id = id;
		this->img = img;
		this->width = width;
		this->height = height;
		this->channels = channels;
	}

	void operator()(atomic_int &row_index, atomic_int &col_index, atomic_bool &done, vector<vector<PixelYCbCr>> &outVector)
	{
		while (row_index.load() <= height || col_index.load() <= width)
		{
			startCol = col_index.fetch_add(8);
			startRow = row_index.load();

			if (startCol >= width)
			{
				if (startRow >= height)
				{
					break;
				}
				else
				{
					// Make sure only one thread increments col_index at a time
					// this seems kinda hacky, but two threads can access row_index around the same time and increment col_index twice
					// resulting in skipped columns. This check ensures only the first thread to go above height can update col and row.
					if (startRow == row_index.load() && startCol < (width + 7))
					{
						row_index += 8;
						col_index.store(0);
					}
				}
			}
			else
			{
				compressImage(outVector);
			}
		}

		done.store(true);
		// cout << "thread " << id << "done" << endl;
	}

private:
	// Runs the encoding algorithm for each 8x8 block of the image
	void compressImage(vector<vector<PixelYCbCr>> &outVector)
	{
		if (startRow >= height || startCol >= width)
			return;

		CreateYCbCrArray(outVector);

		// Take DCT for each component of the image
		vector<int> dctY(64, 0);
		vector<int> dctCb(64, 0);
		vector<int> dctCr(64, 0);

		vector<PixelYCbCr> dctCoefficients(64);
		DCT(outVector, dctCoefficients);
		Quantize(dctCoefficients);

		vector<PixelYCbCr> encodedCoe = zigzagEncoding(dctCoefficients);
		vector<pair<int, int>> rleY = runLengthEncodingY(encodedCoe);
		vector<pair<int, int>> rleCb = runLengthEncodingCb(encodedCoe);
		vector<pair<int, int>> rleCr = runLengthEncodingCr(encodedCoe);

		HuffmanNode *treeY = buildTree(rleY);
		map<int, string> tableY = buildTable(treeY);
		string strY = encodeY(encodedCoe, tableY);

		HuffmanNode *treeCb = buildTree(rleCb);
		map<int, string> tableCb = buildTable(treeCb);
		string strCb = encodeY(encodedCoe, tableCb);

		HuffmanNode *treeCr = buildTree(rleCr);
		map<int, string> tableCr = buildTable(treeCr);
		string strCr = encodeY(encodedCoe, tableCr);
	}

	PixelYCbCr toYCbCr(int R, int G, int B)
	{
		PixelYCbCr outColor = PixelYCbCr();

		outColor.Y = R * 0.229 + G * 0.587 + B * 0.144;
		outColor.Cb = R * -0.168935 + G * -0.331665 + B * 0.50059 + 128;
		outColor.Cr = R * 0.499813 + G * -0.418531 + B * -.081282 + 128;

		return outColor;
	}

	void CreateYCbCrArray(vector<vector<PixelYCbCr>> &outVector)
	{
		long long dataIndex;
		for (int i = startRow; i < (startRow + 8); i++)
		{
			dataIndex = (i * (width * 3)) + (startCol * 3);
			for (int j = startCol; j < (startCol + 8); j++)
			{
				if (i >= height || j >= width)
				{
					outVector[i][j] = toYCbCr(0, 0, 0);
				}
				else
				{
					outVector[i][j] = toYCbCr(img[dataIndex], img[dataIndex + 1], img[dataIndex + 2]);
					dataIndex += 3;
				}
			}
		}
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

	void DCT(vector<vector<PixelYCbCr>> &pixelArray, vector<PixelYCbCr> &out)
	{
		float ci = 0;
		float cj = 0;
		float sumY = 0;
		float sumCb = 0;
		float sumCr = 0;
		float cosk, cosl;

		int index = 0;

		pixelArray[0 + 0][0].Y = 255;
		for (int i = startRow; i < startRow + 8; i++)
		{
			for (int j = startCol; j < startCol + 8; j++)
			{
				ci = i == 0 ? inverseSQR8 : sqr2oversqr8;
				cj = j == 0 ? inverseSQR8 : sqr2oversqr8;

				sumY = 0;
				sumCb = 0;
				sumCr = 0;
				for (int k = 0; k < 8; k++)
				{
					for (int l = 0; l < 8; l++)
					{
						cosk = cos((2 * k + 1) * i * piOver16);
						cosl = cos((2 * l + 1) * j * piOver16);
						sumY += getPixelVal(pixelArray[i][j], 0) * cosk * cosl;
						sumCb += getPixelVal(pixelArray[i][j], 1) * cosk * cosl;
						sumCr += getPixelVal(pixelArray[i][j], 2) * cosk * cosl;
					}
				}
				out[index++] = toYCbCr((ci * cj * sumY), (ci * cj * sumCb), (ci * cj * sumCr));
			}
		}
	}

	// Input: array of DCT coefficients
	// Output: quantized values
	void Quantize(vector<PixelYCbCr> &pixelArray)
	{
		static const int quantizationMatrix[64] =
			{
				16, 11, 10, 16, 24, 40, 51, 61,
				12, 12, 14, 19, 26, 58, 60, 55,
				14, 13, 16, 24, 40, 57, 69, 56,
				14, 17, 22, 29, 51, 87, 80, 62,
				18, 22, 37, 56, 68, 109, 103, 77,
				24, 35, 55, 64, 81, 104, 113, 92,
				49, 64, 78, 87, 103, 121, 120, 101,
				72, 92, 95, 98, 112, 100, 103, 99};

		static const int chrominanceQuantizationMatrix[64] =
			{
				17, 18, 24, 47, 99, 99, 99, 99,
				18, 21, 26, 66, 99, 99, 99, 99,
				24, 26, 56, 99, 99, 99, 99, 99,
				47, 66, 99, 99, 99, 99, 99, 99,
				99, 99, 99, 99, 99, 99, 99, 99,
				99, 99, 99, 99, 99, 99, 99, 99,
				99, 99, 99, 99, 99, 99, 99, 99,
				99, 99, 99, 99, 99, 99, 99, 99};

		for (int i = 0; i < pixelArray.size(); i++)
		{
			pixelArray[i].Y = round((float)pixelArray[i].Y / quantizationMatrix[i]);
			pixelArray[i].Cb = round((float)pixelArray[i].Cb / chrominanceQuantizationMatrix[i]);
			pixelArray[i].Cr = round((float)pixelArray[i].Cr / chrominanceQuantizationMatrix[i]);
		}
	}

	vector<PixelYCbCr> zigzagEncoding(vector<PixelYCbCr> pixelArray)
	{
		vector<PixelYCbCr> result(64);
		static const int zigzag[64] =
			{
				0, 1, 8, 16, 9, 2, 3, 10,
				17, 24, 32, 25, 18, 11, 4, 5,
				12, 19, 26, 33, 40, 48, 41, 34,
				27, 20, 13, 6, 7, 14, 21, 28,
				35, 42, 49, 56, 57, 50, 43, 36,
				29, 22, 15, 23, 30, 37, 44, 51,
				58, 59, 52, 45, 38, 31, 39, 46,
				53, 60, 61, 54, 47, 55, 62, 63};

		for (int i = 0; i < 64; i++)
		{
			result[i] = pixelArray[zigzag[i]];
		}

		return result;
	}

	vector<pair<int, int>> runLengthEncodingY(vector<PixelYCbCr> arr)
	{
		vector<pair<int, int>> result;

		int val = arr[0].Y;
		int count = 1;
		for (int i = 1; i < arr.size(); i++)
		{
			if (val == arr[i].Y)
			{
				count++;
			}
			else
			{
				result.push_back(pair(val, count));
				count = 1;
				val = arr[i].Y;
			}
		}

		result.push_back(pair(val, count));

		return result;
	}

	vector<pair<int, int>> runLengthEncodingCb(vector<PixelYCbCr> arr)
	{
		vector<pair<int, int>> result;

		int val = arr[0].Cb;
		int count = 1;
		for (int i = 1; i < arr.size(); i++)
		{
			if (val == arr[i].Cb)
			{
				count++;
			}
			else
			{
				result.push_back(pair(val, count));
				count = 1;
				val = arr[i].Cb;
			}
		}

		result.push_back(pair(val, count));

		return result;
	}

	vector<pair<int, int>> runLengthEncodingCr(vector<PixelYCbCr> arr)
	{
		vector<pair<int, int>> result;

		int val = arr[0].Cr;
		int count = 1;
		for (int i = 1; i < arr.size(); i++)
		{
			if (val == arr[i].Cr)
			{
				count++;
			}
			else
			{
				result.push_back(pair(val, count));
				count = 1;
				val = arr[i].Cr;
			}
		}

		result.push_back(pair(val, count));

		return result;
	}
};