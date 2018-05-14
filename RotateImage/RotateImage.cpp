// RotateImage.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>



#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

// Avoid Visual Studio LNK2019 compiler error
#pragma comment(lib, "OpenCL.lib")

using namespace std;

int main() {
	const size_t N = 1 << 20;

	try {
		// Get list of OpenCL platforms.
		std::vector<cl::Platform> platform;
		cl::Platform::get(&platform);

		if (platform.empty()) {
			std::cerr << "OpenCL platforms not found." << std::endl;
			return 1;
		}

		// Get first available GPU device which supports double precision.
		cl::Context context;

		// * * * * * * * * * * * * * * * * * * * * *
		// PART I: Populate device vector
		// * * * * * * * * * * * * * * * * * * * * *
		std::vector<cl::Device> device;

		// 1.) Iterate platforms
		for (auto p = platform.begin(); device.empty() && p != platform.end(); p++) {
			std::vector<cl::Device> pldev;

			try {

				// 2.) Get GPU devices per platform
				p->getDevices(CL_DEVICE_TYPE_GPU, &pldev);

				// Iterate devices
				for (auto d = pldev.begin(); device.empty() && d != pldev.end(); d++) {
					if (!d->getInfo<CL_DEVICE_AVAILABLE>()) continue;

					std::string ext = d->getInfo<CL_DEVICE_EXTENSIONS>();

					if (
						ext.find("cl_khr_fp64") == std::string::npos &&
						ext.find("cl_amd_fp64") == std::string::npos
						) continue;

					device.push_back(*d);
					context = cl::Context(device);
				}
			}
			catch (...) {
				device.clear();
			}
		}

		if (device.empty()) {
			std::cerr << "GPUs with double precision not found." << std::endl;
			return 1;
		}

		// Print first device name to console
		std::cout << device[0].getInfo<CL_DEVICE_NAME>() << std::endl;

		// * * * * * * * * * * * * * * * * * * * * *
		// PART II: Compile kernel for device
		// * * * * * * * * * * * * * * * * * * * * *

		/*
		FILE *sourceFP;
		char *sourceText;
		size_t sourceLength;

		errno_t fileOpeningError = fopen_s(&sourceFP, "RotateImage.cl", "rb");
		if (fileOpeningError != 0) {
			std::cerr << "Couldn't open kernel file." << std::endl;
			return 1;
		}
		*/

		// Read kernel file contents
		ifstream kernelFS("RotateImage.cl");
		string kernelSourceString((istreambuf_iterator<char>(kernelFS)), (istreambuf_iterator<char>()));
		char *kernelSource = &kernelSourceString[0u];
		
		// Create command queue. Connect device and context!
		cl::CommandQueue queue(context, device[0]);


		// Compile OpenCL program for found device.
		cl::Program program(context, cl::Program::Sources(
			1, std::make_pair(kernelSource, strlen(kernelSource))
		));

		//cl::Program program = clCreateProgramWithSource(context, 1, );

		try {
			program.build(device);
		}
		catch (const cl::Error&) {
			std::cerr
				<< "OpenCL compilation error" << std::endl
				<< program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device[0])
				<< std::endl;
			return 1;
		}

		cl::Kernel add(program, "add");

		// * * * * * * * * * * * * * * * * * * * * *
		// PART III: Transfer data
		// * * * * * * * * * * * * * * * * * * * * *
		
		// Prepare input data.
		std::vector<double> a(N, 1);
		std::vector<double> b(N, 2);
		std::vector<double> c(N);

		// Allocate device buffers and transfer input data to device.
		cl::Buffer A(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			a.size() * sizeof(double), a.data());

		cl::Buffer B(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			b.size() * sizeof(double), b.data());

		cl::Buffer C(context, CL_MEM_READ_WRITE,
			c.size() * sizeof(double));

		// Set kernel parameters.
		add.setArg(0, static_cast<cl_ulong>(N));
		add.setArg(1, A);
		add.setArg(2, B);
		add.setArg(3, C);

		// Launch kernel on the compute device.
		queue.enqueueNDRangeKernel(add, cl::NullRange, N, cl::NullRange);

		// Get result back to host.
		queue.enqueueReadBuffer(C, CL_TRUE, 0, c.size() * sizeof(double), c.data());

		// Should get '3' here.
		std::cout << c[42] << std::endl;
	}
	catch (const cl::Error &err) {
		std::cerr
			<< "OpenCL error: "
			<< err.what() << "(" << err.err() << ")"
			<< std::endl;
		return 1;
	}
}
