#include <windows.h>
#include <string>
#include <gdiplus.h>
#include <iostream>
 
using namespace std;
using namespace Gdiplus;
 
//#pragma comment(lib,"GdiPlus.lib")
 
 
int main(){

   GdiplusStartupInput gdiplusStartupInput;
   ULONG_PTR gdiplusToken;
   GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

   Image* image1 = new Image(L"bird.png");

   ImageType type1 = image1->GetType();

   if(type1 == ImageTypeBitmap)
      printf("The type of image1 is ImageTypeBitmap.\n");

   if(type1 == ImageTypeMetafile)
      printf("The type of image1 is ImageTypeMetafile.\n");

   delete image1;

   GdiplusShutdown(gdiplusToken);
   return 0;

}