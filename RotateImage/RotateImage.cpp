// RotateImage.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include "tga.h"

// Avoid Visual Studio LNK2019 compiler error
#pragma comment(lib, "OpenCL.lib")

using namespace std;

typedef struct RGB {
	unsigned char R;
	unsigned char G;
	unsigned char B;
} RGB;

int main() {

	cl_int err;
	cl_platform_id platforms[8];
	cl_uint numPlatforms;
	cl_device_id device;
	
	err = clGetPlatformIDs(8, platforms, &numPlatforms);
	if (err != CL_SUCCESS) {
		cerr << "OpenCL platforms not found." << std::endl;
		return 1;
	}

	err = clGetDeviceIDs(platforms[1], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	if (err != CL_SUCCESS) {
		cerr << "Device not found." << std::endl;
		return 1;
	}

	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
	cl_command_queue queue = clCreateCommandQueue(context, device, (cl_command_queue_properties)0, &err);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't create command queue." << std::endl;
		return 1;
	}

	ifstream kernelFS("RotateImage.cl");
	string kernelSourceString((istreambuf_iterator<char>(kernelFS)), (istreambuf_iterator<char>()));
	const char *kernelSource = &kernelSourceString[0u];

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't create program object." << std::endl;
		return 1;
	}

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {

		fprintf(stderr, "Couldn't build program (%d).\n", err);
		return 1;
	}

	cl_kernel rotateImage = clCreateKernel(program, "rotateImage", &err);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't create kernel." << std::endl;
		return 1;
	}

	tga::TGAImage image;
	if (tga::LoadTGA(&image, "lenna.tga") != 1) {
		cerr << "Couldn't load image." << std::endl;
		return 1;
	}

	uint32_t numPixels = image.width * image.height;
	RGB* imageIn = new RGB[numPixels];
	RGB* imageOut = new RGB[numPixels];
	uint32_t j = 0;
	uint32_t imageStructSize = numPixels * sizeof(RGB);

	for (uint32_t i = 0; i < numPixels; i++) {
		imageIn[i].R = image.imageData[j++];
		imageIn[i].G = image.imageData[j++];
		imageIn[i].B = image.imageData[j++];
	}

	cl_mem imageInBuffer = 
		clCreateBuffer(context, CL_MEM_READ_ONLY, imageStructSize, NULL, &err);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't create kernel input buffer." << std::endl;
		return 1;
	}

	cl_mem imageOutBuffer = 
		clCreateBuffer(context, CL_MEM_WRITE_ONLY, imageStructSize, NULL, &err);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't create kernel output buffer." << std::endl;
		return 1;
	}

	err = clEnqueueWriteBuffer(queue, imageInBuffer, CL_TRUE, 0, imageStructSize, imageIn, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't write image input buffer." << std::endl;
		return 1;
	}

	float theta = 90 * CL_M_PI / 180;
	float sinTheta = sin(theta);
	float cosTheta = cos(theta);

	clSetKernelArg(rotateImage, 0, sizeof(cl_mem), (void *)&imageInBuffer);
	clSetKernelArg(rotateImage, 1, sizeof(cl_mem), (void *)&imageOutBuffer);
	clSetKernelArg(rotateImage, 2, sizeof(cl_int), (void *)&image.width);
	clSetKernelArg(rotateImage, 3, sizeof(cl_int), (void *)&image.height);
	clSetKernelArg(rotateImage, 4, sizeof(cl_float), (void *)&sinTheta);
	clSetKernelArg(rotateImage, 5, sizeof(cl_float), (void *)&cosTheta);

	size_t workgroupSize[] = { image.width, image.height };

	err = clEnqueueNDRangeKernel(queue, rotateImage, 2, 0, workgroupSize, NULL, 0, NULL, NULL);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't enqueue command to execute on kernel." << std::endl;
		return 1;
	}

	err = clEnqueueReadBuffer(queue, imageOutBuffer, CL_TRUE, 0, imageStructSize, imageOut, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		cerr << "Couldn't enqueue command to read from kernel." << std::endl;
		return 1;
	}

	if (clFinish(queue) != CL_SUCCESS) {
		cerr << "Couldn't finish queue." << std::endl;
		return 1;
	}

	vector<unsigned char> outPixels;

	for (uint32_t i = 0; i < numPixels; i++) {
		outPixels.push_back(imageOut[i].R);
		outPixels.push_back(imageOut[i].G);
		outPixels.push_back(imageOut[i].B);
	}

	image.imageData = outPixels;
	tga::saveTGA(image, "lenna_rotated.tga");

	return 0;
}