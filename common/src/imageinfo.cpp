#include "imageinfo.h"
#include <iostream>

using namespace std;

ULONG_PTR initialize_image_subsystem(){

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	return gdiplusToken;
}

void shutdown_image_subsystem(ULONG_PTR token){

	Gdiplus::GdiplusShutdown(token);
}

image_t * populate_imageinfo(const char * filename){

	wchar_t * file_wchar = new wchar_t[strlen(filename) + 1];

	mbstowcs(file_wchar, filename, strlen(filename) + 1);

	Gdiplus::Bitmap *image = Gdiplus::Bitmap::FromFile(file_wchar);
	Gdiplus::Status ok;

	image_t * imageinfo = new image_t;

	imageinfo->width = image->GetWidth();
	imageinfo->height = image->GetHeight();

	delete image;

	return imageinfo;

}

#define COLORS 3

//need routines to save and create images

byte * get_image_buffer(Gdiplus::Bitmap * image, uint32_t * height, uint32_t * width, uint32_t * fields){

	Gdiplus::Status ok;

	byte * buffer = new byte[image->GetHeight() * image->GetWidth() * 3];

	*height = image->GetHeight();
	*width = image->GetWidth();
	*fields = COLORS;

	for (int i = 0; i < image->GetWidth(); i++){
		for (int j = 0; j < image->GetHeight(); j++){
			Gdiplus::Color color;
			ok = image->GetPixel(i, j, &color);


			if (ok != 0){
				std::cout << "error" << std::endl;
				exit(-1);
			}
			else{

				uint32_t index = (i + j * image->GetWidth()) * COLORS;
				buffer[index] = color.GetR();
				buffer[index + 1] = color.GetG();
				buffer[index + 2] = color.GetB();

			}

		}
	}

	return buffer;

}

void update_image_buffer(Gdiplus::Bitmap * image, byte * array){

	/* update the bitmap image*/

	for (int i = 0; i < image->GetWidth(); i++){
		for (int j = 0; j < image->GetHeight(); j++){
			Gdiplus::Color color;
			Gdiplus::ARGB value = 0;

			uint32_t index = (i + j * image->GetWidth()) * COLORS;

			value |= ((uint32_t)color.GetA()) << 24;
			value |= (((uint32_t)array[index]) << 16);
			value |= (((uint32_t)array[index + 1]) << 8);
			value |= (((uint32_t)array[index + 2]));

			color.SetValue(value);

			image->SetPixel(i, j, color);


		}
	}

}



int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	ImageCodecInfo* pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure
	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return 0;
}

void save_image(Gdiplus::Bitmap * image, wchar_t * file){

	CLSID clsId;

	GetEncoderClsid(L"image/jpeg", &clsId);

	image->Save(file, &clsId);


}









