#ifndef _IMAGE_MANI_H
#define _IMAGE_MANI_H

#include "static_image.h"
#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <iostream>
#include <stdint.h>
#include <wchar.h>
#include <string>
#include <stdio.h>
#include <algorithm>
#include <string.h>

using namespace std;

#define COLORS	3

ULONG_PTR initialize_image_subsystem();
void shutdown_image_subsystem(ULONG_PTR token);

template<typename T> Image<T> load_image(char * filename);
template<typename T> void save_image(char * filename, Image<T> im);

/******copied from image_io.h **********/

// Convert to u8
inline void convert(uint8_t in, uint8_t &out) { out = in; }
inline void convert(uint16_t in, uint8_t &out) { out = in >> 8; }
inline void convert(uint32_t in, uint8_t &out) { out = in >> 24; }
inline void convert(int8_t in, uint8_t &out) { out = in; }
inline void convert(int16_t in, uint8_t &out) { out = in >> 8; }
inline void convert(int32_t in, uint8_t &out) { out = in >> 24; }
inline void convert(float in, uint8_t &out) { out = (uint8_t)(in*255.0f); }
inline void convert(double in, uint8_t &out) { out = (uint8_t)(in*255.0f); }

// Convert to u16
inline void convert(uint8_t in, uint16_t &out) { out = in << 8; }
inline void convert(uint16_t in, uint16_t &out) { out = in; }
inline void convert(uint32_t in, uint16_t &out) { out = in >> 16; }
inline void convert(int8_t in, uint16_t &out) { out = in << 8; }
inline void convert(int16_t in, uint16_t &out) { out = in; }
inline void convert(int32_t in, uint16_t &out) { out = in >> 16; }
inline void convert(float in, uint16_t &out) { out = (uint16_t)(in*65535.0f); }
inline void convert(double in, uint16_t &out) { out = (uint16_t)(in*65535.0f); }

// Convert from u8
inline void convert(uint8_t in, uint32_t &out) { out = in << 24; }
inline void convert(uint8_t in, int8_t &out) { out = in; }
inline void convert(uint8_t in, int16_t &out) { out = in << 8; }
inline void convert(uint8_t in, int32_t &out) { out = in << 24; }
inline void convert(uint8_t in, float &out) { out = in / 255.0f; }
inline void convert(uint8_t in, double &out) { out = in / 255.0f; }

// Convert from u16
inline void convert(uint16_t in, uint32_t &out) { out = in << 16; }
inline void convert(uint16_t in, int8_t &out) { out = in >> 8; }
inline void convert(uint16_t in, int16_t &out) { out = in; }
inline void convert(uint16_t in, int32_t &out) { out = in << 16; }
inline void convert(uint16_t in, float &out) { out = in / 65535.0f; }
inline void convert(uint16_t in, double &out) { out = in / 65535.0f; }


inline bool ends_with_ignore_case(std::string a, std::string b) {
	if (a.length() < b.length()) { return false; }
	std::transform(a.begin(), a.end(), a.begin(), ::tolower);
	std::transform(b.begin(), b.end(), b.begin(), ::tolower);
	return a.compare(a.length() - b.length(), b.length(), b) == 0;
}



inline int is_little_endian() {
	int value = 1;
	return ((char *)&value)[0] == 1;
}

#define SWAP_ENDIAN16(little_endian, value) if (little_endian) { (value) = (((value) & 0xff)<<8)|(((value) & 0xff00)>>8); }

/*** end of image_io.h copy ******/


ULONG_PTR initialize_image_subsystem(){

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	return gdiplusToken;
}

void shutdown_image_subsystem(ULONG_PTR token){

	Gdiplus::GdiplusShutdown(token);
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

void save_image(Gdiplus::Bitmap * image, char * file){

	CLSID clsId;

	wchar_t * file_wchar = new wchar_t[strlen(file) + 1];;
	mbstowcs(file_wchar, file, strlen(file) + 1);

	string filename(file);

	string extension = "";
	int index = -1;
	for (int i = filename.size() - 1; i >= 0; i--){
		if (filename[i] == '.'){
			index = i + 1;
			break;
		}
	}

	if (index != -1){

		for (int i = index; i<filename.size(); i++){
			extension += filename[i];
		}

	}

	/*image/bmp
	image/jpeg
	image/gif
	image/tiff
	image/png*/

	if ((extension.compare("jpg") == 0)){
		GetEncoderClsid(L"image/jpeg", &clsId);
	}
	else if ((extension.compare("png") == 0)){
		GetEncoderClsid(L"image/png", &clsId);
	}
	else if ((extension.compare("bmp")) == 0){
		GetEncoderClsid(L"image/bmp", &clsId);
	}
	else{
		cout << "error: unknown image type" << endl;
		return;
	}

	image->Save(file_wchar, &clsId);

}

template<typename T>
Image<T> load_image(char * filename){


	wchar_t * file_wchar = new wchar_t[strlen(filename) + 1];;
	mbstowcs(file_wchar, filename, strlen(filename) + 1);

	Gdiplus::Bitmap *image = Gdiplus::Bitmap::FromFile(file_wchar);

	Gdiplus::Status ok;

	int width, height;
	width = image->GetWidth();
	height = image->GetHeight();

	Image<T> im(COLORS, width, height);

	T *im_data = (T*)im.data();

	for (int j = 0; j< height; j++){
		for (int i = 0; i< width; i++){	

			Gdiplus::Color color;
			ok = image->GetPixel(i, j, &color);

			/* we are getting byte-wise color so no need to differentiate 8 and 16 */
			/*convert(color.GetR(), im_data[(0 * height + j)*width + i]);
			convert(color.GetG(), im_data[(1 * height + j)*width + i]);
			convert(color.GetB(), im_data[(2 * height + j)*width + i]);*/

			im_data[j*width*3 + i*3 + 0] = color.GetR();
			im_data[j*width*3 + i*3 + 1] = color.GetG();
			im_data[j*width*3 + i*3 + 2] = color.GetB();


		}
	}

	delete image;

	im(0, 0, 0) = im(0, 0, 0);
	return im;

}

template<typename T>
void save_image(char * filename, Image<T> im){

	int width, height, channels;
	width = im.height();
	height = im.channels();
	channels = 3;

	Gdiplus::Bitmap * image = new Gdiplus::Bitmap(width,height);

	Gdiplus::Status ok;

	for (int j = 0; j<height; j++){
		for (int i = 0; i< width; i++){

			Gdiplus::Color color;
			Gdiplus::ARGB value = 0;

			uint8_t color_val;


			value |= ((uint32_t)255) << 24;  /* we don't care about this */
			for (int k = 0; k < COLORS; k++){
				color_val = im((channels <= k) ? channels - 1 : k,i, j);
				/*convert(im(i, j, (channels <= k) ? channels - 1 : k), color_val);*/
				value |= ((uint32_t)color_val) << (16 - 8 * k);
			}


			color.SetValue(value);

			image->SetPixel(i, j, color);

		}
	}

	save_image(image, filename);

	delete image;


}




#endif