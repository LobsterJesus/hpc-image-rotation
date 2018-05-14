#include "tga.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

// Uncompressed TGA Header
const unsigned char uTGAcompare[12] = {0,0, 2,0,0,0,0,0,0,0,0,0};
// Compressed TGA Header
const unsigned char cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};


bool tga::saveTGA(const TGAImage& image, const char * filename)
{
	const unsigned int bytesPerPixel = image.bpp / 8;

	std::ofstream myfile;
	myfile.open (filename, std::ios::out | std::ios::trunc | std::ios::binary);
	//create the tga header
	unsigned char header[6];
	// 1x1 image
	header[0] = image.width % 256;
	header[1] = image.width / 256;
	header[2] = image.height % 256;
	header[3] = image.height / 256;
	header[4] = image.bpp;
	header[5] = image.bpp == 32 ? 8 : 0; //flag alpha depth and other flags

	//add the headers
	for (int i = 0; i < 12; ++i)
	{
		myfile << uTGAcompare[i];
	}
	for (int i = 0; i < 6; ++i)
	{
		myfile << header[i];
	}
	//myfile << header;
	//add the image data
	std::vector<unsigned char> imageData(image.imageData); 

	//swap from RGB to BGR
	// Start The Loop
	for(unsigned int cswap = 0; cswap < imageData.size(); cswap += bytesPerPixel)
	{
		// 1st Byte XOR 3rd Byte XOR 1st Byte XOR 3rd Byte
		imageData[cswap] ^= imageData[cswap+2] ^=
		imageData[cswap] ^= imageData[cswap+2];
	}

	for (int i = 0; i < imageData.size(); ++i)
	{
		unsigned char x = imageData[i];
		myfile << imageData[i];
	}

	myfile.close();

	return true;
}

// Load A TGA File!
bool tga::LoadTGA(TGAImage * image, const char * filename)
{

	tga::TGAHeader tgaheader;				// Used To Store Our File Header
	tga::TGA tga_;					// Used To Store File Information
	
	FILE * fTGA;					// Declare File Pointer
	fTGA = fopen(filename, "rb");			// Open File For Reading

	if(fTGA == NULL)				// If Here Was An Error
	{
		std::cout << "loadTGA: error reading file " << filename << std::endl;
		return false;				// Return False
	}

	// Attempt To Read The File Header
	if(fread(&tgaheader, sizeof(tga::TGAHeader), 1, fTGA) == 0)
	{
		printf("error trying to read the tga file header\n");
		fclose(fTGA);
		return false;				// Return False If It Fails
	}
	


		// If The File Header Matches The Uncompressed Header
	if(memcmp(uTGAcompare, &tgaheader, sizeof(tgaheader)) == 0)
	{
		// Load An Uncompressed TGA
		LoadUncompressedTGA(image, filename, fTGA, tgaheader, tga_);
	}
	// If The File Header Matches The Compressed Header
	else if(memcmp(cTGAcompare, &tgaheader, sizeof(tgaheader)) == 0)
	{
		// Load A Compressed TGA
		LoadCompressedTGA(image, filename, fTGA, tgaheader, tga_);
	}
	else						// If It Doesn't Match Either One
	{
		std::cout << "loadTGA: error: tga file header does not match\n";
		return false;				// Return False
	}

	return true;
}

// Load An Uncompressed TGA
bool tga::LoadUncompressedTGA(tga::TGAImage * image, const char * filename, FILE * fTGA, tga::TGAHeader& tgaheader, tga::TGA& tga_){

	// Attempt To Read Next 6 Bytes
	if(fread(tga_.header, sizeof(tga_.header), 1, fTGA) == 0)
	{
		std::cout << "loadTGA: error reading the next 6 bytes of the TGA\n" ;
		return false;				// Return False
	}
	
	image->width  = tga_.header[1] * 256 + tga_.header[0];	// Calculate Height
	image->height = tga_.header[3] * 256 + tga_.header[2];	// Calculate The Width
	image->bpp = tga_.header[4];				// Calculate Bits Per Pixel
	tga_.Width = image->width;				// Copy Width Into Local Structure
	tga_.Height = image->height;				// Copy Height Into Local Structure
	tga_.Bpp = image->bpp;					// Copy Bpp Into Local Structure

	// Make Sure All Information Is Valid
	if((image->width <= 0) || (image->height <= 0) || ((image->bpp != 24) && (image->bpp !=32)))
	{
		std::cout << "loadTGA: error: width/height or bbp invalid\n" ;
		return false;				// Return False
	}

	if(image->bpp == 24)				// Is It A 24bpp Image?
		image->type	= 0;		// If So, Set Type To GL_RGB
	else						// If It's Not 24, It Must Be 32
		image->type	= 1;		// So Set The Type To GL_RGBA

	tga_.bytesPerPixel = (tga_.Bpp / 8);		// Calculate The BYTES Per Pixel
	// Calculate Memory Needed To Store Image
	tga_.imageSize = (tga_.bytesPerPixel * tga_.Width * tga_.Height);

	// Allocate Memory
	image->imageData = std::vector<unsigned char>(tga_.imageSize);//(char *)malloc(tga_.imageSize);
	//if(image->imageData == NULL)			// Make Sure It Was Allocated Ok
	//{
	//	std::cout << "loadTGA: error: image data was not allocated properly\n";
	//	return false;				// If Not, Return False
	//}

	// Attempt To Read All The Image Data
	if(fread(&image->imageData[0], 1, tga_.imageSize, fTGA) != tga_.imageSize)
	{
		std::cout << "loadTGA: error reading image data\n";
		return false;				// If We Cant, Return False
	}

	//swap from BGR to RGB
	// Start The Loop
	for(unsigned int cswap = 0; cswap < tga_.imageSize; cswap += tga_.bytesPerPixel)
	{
		// 1st Byte XOR 3rd Byte XOR 1st Byte XOR 3rd Byte
		image->imageData[cswap] ^= image->imageData[cswap+2] ^=
		image->imageData[cswap] ^= image->imageData[cswap+2];
	}

	fclose(fTGA);					// Close The File
	return true;					// Return Success

}

bool tga::LoadCompressedTGA(tga::TGAImage * image, const char * filename, FILE * fTGA, tga::TGAHeader& tgaheader, tga::TGA& tga) {

	// Attempt To Read Next 6 Bytes
	if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)
	{
		std::cout << "loadTGA: error reading the next 6 bytes of the TGA\n" ;
		return false;				// Return False
	}

	image->width  = tga.header[1] * 256 + tga.header[0];	// Calculate Height
	image->height = tga.header[3] * 256 + tga.header[2];	// Calculate The Width
	image->bpp = tga.header[4];				// Calculate Bits Per Pixel
	tga.Width = image->width;				// Copy Width Into Local Structure
	tga.Height = image->height;				// Copy Height Into Local Structure
	tga.Bpp = image->bpp;					// Copy Bpp Into Local Structure

	// Make Sure All Information Is Valid
	if((image->width <= 0) || (image->height <= 0) || ((image->bpp != 24) && (image->bpp !=32)))
	{
		std::cout << "loadTGA: error: width/height or bbp invalid\n" ;
		return false;				// Return False
	}

	if(image->bpp == 24)				// Is It A 24bpp Image?
		image->type	= 0;		// If So, Set Type To GL_RGB
	else						// If It's Not 24, It Must Be 32
		image->type	= 1;		// So Set The Type To GL_RGBA

	tga.bytesPerPixel = (tga.Bpp / 8);		// Calculate The BYTES Per Pixel
	// Calculate Memory Needed To Store Image
	tga.imageSize = (tga.bytesPerPixel * tga.Width * tga.Height);

	// Allocate Memory To Store Image Data
	image->imageData	= std::vector<unsigned char>(tga.imageSize);

	unsigned int pixelcount = tga.Height * tga.Width;	// Number Of Pixels In The Image
	unsigned int currentpixel	= 0;			// Current Pixel We Are Reading From Data
	unsigned int currentbyte	= 0;			// Current Byte We Are Writing Into Imagedata
	// Storage For 1 Pixel
	unsigned char * colorbuffer = (unsigned char *)malloc(tga.bytesPerPixel);

	do						// Start Loop
	{
	unsigned char chunkheader = 0;			// Variable To Store The Value Of The Id Chunk
	if(fread(&chunkheader, sizeof(unsigned char), 1, fTGA) == 0)	// Attempt To Read The Chunk's Header
	{
		std::cout << "loadTGA: error reading chunk header \n";
		free(colorbuffer);
		return false;				// If It Fails, Return False
	}

	if(chunkheader < 128)				// If The Chunk Is A 'RAW' Chunk
	{
		chunkheader++;				// Add 1 To The Value To Get Total Number Of Raw Pixels

	// Start Pixel Reading Loop
	for(short counter = 0; counter < chunkheader; counter++)
	{
		// Try To Read 1 Pixel
		if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel)
		{
			std::cout << "loadTGA: error reading a single pixel\n";
			free(colorbuffer);
			return false;			// If It Fails, Return False
		}
	image->imageData[currentbyte] = colorbuffer[2];		// Write The 'R' Byte
	image->imageData[currentbyte + 1	] = colorbuffer[1];	// Write The 'G' Byte
	image->imageData[currentbyte + 2	] = colorbuffer[0];	// Write The 'B' Byte
	if(tga.bytesPerPixel == 4)					// If It's A 32bpp Image...
	{
		image->imageData[currentbyte + 3] = colorbuffer[3];	// Write The 'A' Byte
	}
	// Increment The Byte Counter By The Number Of Bytes In A Pixel
	currentbyte += tga.bytesPerPixel;
	currentpixel++;					// Increment The Number Of Pixels By 1
	}
	}
	else						// If It's An RLE Header
	{
		chunkheader -= 127;			// Subtract 127 To Get Rid Of The ID Bit
	// Read The Next Pixel
	if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel)
	{
		std::cout << "loadTGA: error reading a single pixel\n";
		free(colorbuffer);
		return false;				// If It Fails, Return False
	}
	// Start The Loop
	for(short counter = 0; counter < chunkheader; counter++)
	{
		// Copy The 'R' Byte
		image->imageData[currentbyte] = colorbuffer[2];
		// Copy The 'G' Byte
		image->imageData[currentbyte + 1	] = colorbuffer[1];
		// Copy The 'B' Byte
		image->imageData[currentbyte + 2	] = colorbuffer[0];
		if(tga.bytesPerPixel == 4)		// If It's A 32bpp Image
		{
			// Copy The 'A' Byte
			image->imageData[currentbyte + 3] = colorbuffer[3];
		}
		currentbyte += tga.bytesPerPixel;	// Increment The Byte Counter
		currentpixel++;				// Increment The Pixel Counter
	}
	}

	}while(currentpixel < pixelcount);

	fclose(fTGA);
	free(colorbuffer);
	return true;

}
