typedef struct RGB {
	unsigned char R;
	unsigned char G;
	unsigned char B;
} RGB;

__kernel void rotateImage(
	__global RGB *sourceBuffer,
	__global RGB *destinationBuffer,
	int imageWidth,
	int imageHeight,
	float sinTheta, 
	float cosTheta)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int centerX = imageWidth / 2;
	const int centerY = imageHeight / 2;
	const int x2 = cosTheta * (x - centerX) - sinTheta * (y - centerY) + centerX;
	const int y2 = sinTheta * (x - centerX) + cosTheta * (y - centerY) + centerY;

	if (x2 >= 0 && x2 < imageWidth && y2 >= 0 && y2 < imageHeight)  
		destinationBuffer[y * imageWidth + x] = sourceBuffer[y2 * imageWidth + x2];
}