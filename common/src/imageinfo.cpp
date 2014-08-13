#include "imageinfo.h"

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











