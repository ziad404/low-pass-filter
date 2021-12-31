#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}

bool check(int i, int j,int height,int width,int totalH) {
	return(i >= 0 && j >= 0 && i <= height && j < width&& i < totalH);
}

int* applyKernel(float kernal[3][3], int* arr, int numOfPixels,int width,int height,int base,int totalH) {
	int dex[] = { -1, -1, -1, 0, 0, 1, 1, 1 }, dey[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	int* imgData = new int[numOfPixels];
	for (int i = 0; i < numOfPixels; i++) {
		float ans = 0;
		int x = i / width;
		int y = i % width;
		int cenX = 1, cenY = 1;
		for (int j = 0; j < 8; j++) {
			if (check(x + dex[j], y + dey[j],height,width,totalH)) {
				int tmp = (x + dex[j]) * width;
				tmp += (y + dey[j]);
				ans += arr[tmp] /9;
			}
		}
		ans += arr[i] / 9;
		imgData[i] = ans;
	}
	return imgData;

}

int createNewImage(int* newPixels, int* newImg,int numOfPixels,int count) {
	cout << numOfPixels << count;
	for (int i = 0; i < numOfPixels; i++) {
		newImg[count] = newPixels[i];
		count++;
	}
	return count;
}

int main()
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0;

	System::String^ imagePath;
	std::string img;
	img = "..//Data//Input//test.png";

	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	start_s = clock();
	MPI_Init(NULL, NULL);
	int numOfProcessors;
	int processorId;
	MPI_Comm_size(MPI_COMM_WORLD, &numOfProcessors);
	MPI_Comm_rank(MPI_COMM_WORLD, &processorId);
	float kernal[3][3];
	int* newImage = new int[ImageHeight * ImageWidth];
	int numOfPixels = ImageHeight * ImageWidth;
	int numOfPixelsPerProcessor = numOfPixels / numOfProcessors;
	int* newLocalImgData = new int[numOfPixelsPerProcessor];
	int check = numOfPixelsPerProcessor % ImageWidth;
	if (check != 0)
		numOfPixelsPerProcessor +=  (ImageWidth - check);
	if (processorId == 0) {
		for (int i = 1; i < numOfProcessors; i++) {
			if (i == numOfProcessors - 1)
				MPI_Send(&imageData[(i * numOfPixelsPerProcessor) - ImageWidth], (numOfPixels - (numOfPixelsPerProcessor * (numOfProcessors-1)) + ImageWidth), MPI_INT, i, 0, MPI_COMM_WORLD);
			else
				MPI_Send(&imageData[(i * numOfPixelsPerProcessor) - ImageWidth], (numOfPixelsPerProcessor + (ImageWidth* 2)), MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		newLocalImgData = applyKernel(kernal, imageData,numOfPixelsPerProcessor, ImageWidth,numOfPixelsPerProcessor/ImageWidth,processorId*numOfPixelsPerProcessor,ImageHeight);
	}
	else if (processorId < numOfProcessors && processorId != 0) {
		int* localImgData = new int[numOfPixels];
		MPI_Status x_status;
		if (processorId == (numOfProcessors - 1)) {
			MPI_Recv(localImgData, (numOfPixels - (numOfPixelsPerProcessor * (numOfProcessors - 1)) + ImageWidth), MPI_INT, 0, 0, MPI_COMM_WORLD, &x_status);
		}
		else
		{
			MPI_Recv(localImgData, (numOfPixelsPerProcessor + (ImageWidth * 2)), MPI_INT, 0, 0, MPI_COMM_WORLD, &x_status);
		}
		newLocalImgData = applyKernel(kernal, localImgData,numOfPixelsPerProcessor, ImageWidth, numOfPixelsPerProcessor / ImageWidth, processorId * numOfPixelsPerProcessor,ImageHeight);
	}
	cout << "Start";
	MPI_Gather(newLocalImgData, numOfPixelsPerProcessor, MPI_INT, newImage, numOfPixelsPerProcessor, MPI_INT, 0, MPI_COMM_WORLD);
	cout << "DONE";
	if(processorId == 0)
		createImage(newImage, ImageWidth, ImageHeight, 1);
	MPI_Finalize();
	cout << "finished" << endl;
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
	cout << "time: " << TotalTime << endl;
	free(imageData);
	return 0;

}



