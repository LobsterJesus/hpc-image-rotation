// simple tga loading and saving
// based on NeHe tutorial 33


#ifndef __TEXTURE_H__				// See If The Header Has Been Defined Yet
#define __TEXTURE_H__				// If Not, Define It.

// disable visual studio warning
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>				// Standard Header For File I/O
#include <vector>

namespace tga{

typedef struct
{
        std::vector<unsigned char> imageData;			// Hold All The Color Values For The Image.
        unsigned int  bpp;				// Hold The Number Of Bits Per Pixel.
        unsigned int width;				// The Width Of The Entire Image.
        unsigned int height;				// The Height Of The Entire Image.
        unsigned int type;			 	// Data Stored In * ImageData (GL_RGB Or GL_RGBA)
} TGAImage;

typedef struct
{
        unsigned char Header[12];			// File Header To Determine File Type
} TGAHeader;

typedef struct
{
        unsigned char header[6];			// Holds The First 6 Useful Bytes Of The File
        unsigned int bytesPerPixel;			// Number Of BYTES Per Pixel (3 Or 4)
        unsigned int imageSize;			// Amount Of Memory Needed To Hold The Image
        unsigned int type;				// The Type Of Image, GL_RGB Or GL_RGBA
        unsigned int Height;				// Height Of Image
        unsigned int Width;				// Width Of Image
        unsigned int Bpp;				// Number Of BITS Per Pixel (24 Or 32)
} TGA;

bool saveTGA(const TGAImage& image, const char * filename); //save as uncompressed tga

bool LoadTGA(TGAImage* image, const char * filename);
// Load An Uncompressed File
bool LoadUncompressedTGA(TGAImage *, const char *, FILE *, tga::TGAHeader&, tga::TGA&);
// Load A Compressed File
bool LoadCompressedTGA(TGAImage *, const char *, FILE *, tga::TGAHeader&, tga::TGA&);

}
#endif
