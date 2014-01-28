#include <iostream>
#include <vector>
#include <IL\il.h>
#include <IL\ilu.h>

typedef unsigned int uint32;

struct pixeldata
{
	ILubyte r;
	ILubyte g;
	ILubyte b;

	pixeldata() : r(0), g(0), b(0) {};

} black;

struct coords
{
	uint32 x;
	uint32 y;

	coords() : x(0), y(0) {};
};

/* Returns true if given coordinate has a white pixel. */
bool IsWhite(uint32 x, uint32 y)
{
	pixeldata p;
	ilCopyPixels(x, y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &p);

	if(
		p.r >= 0xc0 &&
		p.g >= 0xc0 &&
		p.b >= 0xc0) return true;

	return false;
}

/* Will find number of whites in all nodes around given coordinate. */
uint32 FriendCount(uint32 x, uint32 y)
{
	uint32 count = 0;

	if(IsWhite(x-1, y-1)) count++;
	if(IsWhite(x-1, y-0)) count++;
	if(IsWhite(x-1, y+1)) count++;
	if(IsWhite(x+1, y-1)) count++;
	if(IsWhite(x+1, y-0)) count++;
	if(IsWhite(x+1, y+1)) count++;
	if(IsWhite(x+0, y-1)) count++;
	if(IsWhite(x+0, y+1)) count++;

	return count;
}

/* Removes any white pixel with less than 3 neighbouring white nodes. */
void CleanDots(uint32 imWidth, uint32 imHeight)
{
	for(uint32 x = 0; x < imWidth; x++) {
		for(uint32 y = 0; y < imHeight; y++) {
			if(IsWhite(x, y) && FriendCount(x, y) <= 2) {
				ilSetPixels(x, y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);
			}
		}
	}
}

/* Cleans up remaining pixels at edges of image. */
void CleanEdges(uint32 imWidth, uint32 imHeight)
{
	for(uint32 x = 0; x < imWidth; x++) {
		ilSetPixels(x, 0, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);
		ilSetPixels(x, (imHeight-1), 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);
	}
	for(uint32 y = 0; y < imHeight; y++) {
		ilSetPixels(0, y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);
		ilSetPixels((imWidth-1), y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);
	}
}

/* Uses the floodfill algorithm to build our coordinate vector. */
void FloodFill(uint32 x, uint32 y, std::vector<coords>& coordinates, uint32& nodecount)
{
	if(!IsWhite(x, y)) return;
	ilSetPixels(x, y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &black);

	coords coord;
	coord.x = x; coord.y = y;
	coordinates.push_back(coord);

	nodecount++;

	/* Execute FloodFill for neighbouring nodes. */
	FloodFill(x-1, y-1, coordinates, nodecount);
	FloodFill(x-1, y-0, coordinates, nodecount);
	FloodFill(x-1, y+1, coordinates, nodecount);
	FloodFill(x+1, y-1, coordinates, nodecount);
	FloodFill(x+1, y+0, coordinates, nodecount);
	FloodFill(x+1, y+1, coordinates, nodecount);
	FloodFill(x+0, y-1, coordinates, nodecount);
	FloodFill(x+0, y+1, coordinates, nodecount);
}

/* Builds our character vector and more. */
void FindLow(uint32 imWidth, uint32 imHeight)
{
	pixeldata white;
	white.r = 0xff; white.g = 0xff; white.b = 0xff;

	/* Build our character vector. */
	std::vector< std::vector<coords> > characters;
	for(uint32 x = 0; x < imWidth; x++) {
		for(uint32 y = 0; y < imHeight; y++) {
			if(IsWhite(x, y)) {
				uint32 nodecount = 0;
				std::vector<coords> coordinates;
				FloodFill(x, y, coordinates, nodecount);
				if(nodecount > 30) {
					characters.push_back(coordinates);
				}
			}
		}
	}

	std::vector< std::vector<coords> >::iterator iter;
	for(iter = characters.begin(); iter != characters.end(); iter++)
	{
		std::vector<coords>* character = &*iter;
		std::vector<coords>::iterator chariter = (*character).begin();

		/* find our lowest x and highest y value */
		coords corner, bounds; corner.x = imWidth; corner.y = imHeight;
		for(; chariter != (*character).end(); chariter++) {
			if(chariter->x < corner.x) corner.x = chariter->x;
			if(chariter->y < corner.y) corner.y = chariter->y;
		}
		/* now position our character to the far bottom left: */
		for(chariter = (*character).begin(); chariter != (*character).end(); chariter++) {
			chariter->x -= corner.x;
			chariter->y -= corner.y;
		}
	}

	/* Copy back a character to analyze. */
	std::vector<coords>* character = &characters.at(5);
	std::vector<coords>::iterator chariter = (*character).begin();
	for(; chariter != (*character).end(); chariter++)
	{
		// dump our table
		printf("%d, %d\n", chariter->x, chariter->y);
		ilSetPixels(chariter->x, chariter->y, 0, 1, 1, 1, IL_RGB, IL_UNSIGNED_BYTE, &white);
	}
}

int main()
{
	/* Initialize DevIL and load our image. */
	ilInit();

	system("del C:\\out.bmp");

	if(!ilLoad(IL_BMP, "C:\\captcha\\test1.bmp")) {
		printf("Could not load image: %d\n", ilGetError());
		std::cin.get();
		return 0;
	}

	/* Grab some information... */
	uint32 imWidth  = ilGetInteger(IL_IMAGE_WIDTH);
	uint32 imHeight = ilGetInteger(IL_IMAGE_HEIGHT);
	uint32 imPixCnt = imWidth * imHeight * 3;

	/* Convert raw image data to RGB. */
	if(!ilConvertImage(IL_RGB, ilGetInteger(IL_IMAGE_TYPE))) {
		printf("Could not convert image: %d\n", ilGetError());
		std::cin.get();
		return 0;
	}

	/* Clean up. */
	CleanEdges(imWidth, imHeight);
	CleanDots (imWidth, imHeight);

	FindLow(imWidth, imHeight);

	/* Save resulting image. */
	if(!ilSave(IL_BMP, "C:\\out.bmp")) {
		printf("Could not save image: %d\n", ilGetError());
		std::cin.get();
		return 0;
	}

	system("pause");
	return 0;
}
