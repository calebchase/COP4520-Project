#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <atomic>
#include <algorithm>
using namespace std;

struct PixelYCbCr
{
	int Y;
	int Cb;
	int Cr;
};

class CompressorThread
{
private:
    int id;
    unsigned char* img;
    int width;
    int height;
    int channels;

    int startRow;
    int startCol;

public:
    CompressorThread(int id, unsigned char* img, int width, int height, int channels)
    {
        this->id = id;
        this->img = img;
        this->width = width;
        this->height = height;
        this->channels = channels;
    }

    void operator()(atomic_int& row_index, atomic_int& col_index, atomic_bool& done, vector<vector<PixelYCbCr>>& outVector)
    {
        while(row_index.load() <= height || col_index.load() <= width)
        {
            startRow = row_index.fetch_add(8);
            startCol = col_index.load();

            if(startRow >= height)
            {
                if(startCol >= width)
                {
                    break;
                }
                else
                {
                    // Make sure only one thread increments col_index at a time
                    // this seems kinda hacky, but two threads can access row_index around the same time and increment col_index twice
                    // resulting in skipped columns. This check ensures only the first thread to go above height can update col and row.
                    if(startCol == col_index.load() && startRow < (height + 8))
                    {
                        col_index += 8;
                        row_index.store(0);
                    }
                }
            }
            else
            {
                CreateYCbCrArray(outVector, channels);

            }

        }

        done.store(true);
        cout << "thread " << id << "done" << endl;
    }

private:
    PixelYCbCr toYCbCr(int R, int G, int B)
    {
        PixelYCbCr outColor = PixelYCbCr();

        outColor.Y = R * 0.229 + G * 0.587 + B * 0.144;
        outColor.Cb = R * -0.168935 + G * -0.331665 + B * 0.50059 + 128;
        outColor.Cr = R * 0.499813 + G * -0.418531 + B * -.081282 + 128;

        return outColor;
    }

    void CreateYCbCrArray(vector<vector<PixelYCbCr>>& outVector, int channels)
    {
        if(startRow >= height || startCol >= width)
            return;

        long long dataIndex;
        for (int i = startRow; i < (startRow + 8); i++)
        {
            dataIndex = (i * (width * 3)) + (startCol * 3);
            for (int j = startCol; j < (startCol + 8); j++)
            {
                if(i >= height || j >= width)
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

};