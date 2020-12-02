#include <iostream>
#include <cstring>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "GeometricTransformer.h"
#include "TypeConvert.h"
using namespace cv;



int main(int argc, char* argv[]) 
{

	PixelInterpolate* interpolator = nullptr;

	try {
		if (argc < 5)
			throw std::string("Invalid Syntax");

		char* command = argv[1];
		char* interpolateOption = argv[2];
		char* inputPath = argv[3];


		if (strcmp(interpolateOption, "--bl") == 0) 
		{
			interpolator = new BilinearInterpolate();
		}
		else if (strcmp(interpolateOption, "--nn") == 0)
		{
			interpolator = new NearestNeighborInterpolate();
		}
		else
		{
			throw std::string("No such interpolator ") + interpolateOption;
		}

		GeometricTransformer transformer;
		Mat orgImage = imread(inputPath, CV_LOAD_IMAGE_COLOR);
		Mat resultImage;
		int result;

		if (strcmp(command, "--zoom") == 0) 
		{
			float scale = (float)ParseDouble(argv[4]);
			result = transformer.Scale(orgImage, resultImage, scale, scale, interpolator);
		}
		else if (strcmp(command, "--resize") == 0)
		{
			int newWidth = ParseInt(argv[4]);
			int newHeight = ParseInt(argv[5]);

			if (newWidth <= 0 || newHeight <= 0) 
			{
				throw std::string("Invalid image size");
			}

			result = transformer.Resize(orgImage, resultImage, newWidth, newHeight, interpolator);
		}
		else if (strcmp(command, "--rotate") == 0)
		{
			float angle = (float)ParseDouble(argv[4]);

			result = transformer.RotateKeepImage(orgImage, resultImage, angle, interpolator);
		}
		else if (strcmp(command, "--rotateN") == 0)
		{
			float angle = (float)ParseDouble(argv[4]);

			result = transformer.RotateUnkeepImage(orgImage, resultImage, angle, interpolator);
		}
		else if (strcmp(command, "--flip") == 0)
		{
			int axis;
			if (strcmp(argv[4], "Ox") == 0)
				axis = 1;
			else if (strcmp(argv[4], "Oy") == 0)
				axis = 0;
			else
				throw std::string("Invalid axis");

			result = transformer.Flip(orgImage, resultImage, axis, interpolator);
		}
		else 
		{
			throw std::string("No such command ") + command;
		}

		if (result == 0) {
			throw std::string("Cannot perform operation. Maybe due to unexisted image or invalid command arguments");
		}

		namedWindow("Result");
		namedWindow("Original");
		imshow("Result", resultImage);
		imshow("Original", orgImage);
		waitKey(0);
	}
	catch (std::string errMsg) {
		std::cout << errMsg;
	}

	if (interpolator != nullptr)
		delete interpolator;

	return 0;
}