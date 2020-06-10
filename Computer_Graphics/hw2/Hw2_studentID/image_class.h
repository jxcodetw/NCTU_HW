//Texture相關
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>	//要跟它借用BITMAPFILEHEADER這個資料結構
#include "../GL/glut.h"
#include <iostream>


//---------圖片物件---------
class image_class
{
public:
	image_class(const char*);
	~image_class()
	{
		if( image!=NULL )
		{
			operator delete(image);
			glDeleteTextures(1,&my_texture_id);
		}
	}
	void BuildTexture()			//在OpenGL建立材質貼圖
	{
		glBindTexture(GL_TEXTURE_2D,my_texture_id);
	}

	int width;					//圖片的寬度
	int height;					//圖片的高度
	unsigned char *image;		//指向一個陣列，是能直接讓OpenGL使用的圖片資料了
	GLuint my_texture_id;
	
};

//---------簡略的讀取BMP方法---------
image_class::image_class(const char* file_name)
{
	FILE				*fp;
	BITMAPFILEHEADER	mapHeader;			//BMP檔的檔頭結構，得到這結構才能知道Info的大小
	BITMAPINFO			mapInfo;			//得到這Info結構才能知道圖片的大小、長寬

	unsigned int		InfoSize;			//Info的size
	unsigned int		size;				//圖片的size

	fp = fopen(file_name, "rb");				//開啟bmp檔案
	if( fp==NULL )
	{
		std::cout<<"no such file,load bmp failed.\n";
		system("pause");
		exit(-1);
	}

	fread(&mapHeader, sizeof(mapHeader), 1, fp);					//讀取BMP檔的檔頭
	InfoSize = mapHeader.bfOffBits - sizeof(mapHeader);				//得知Info的size
	fread(&mapInfo, InfoSize, 1, fp);								//讀取BMP檔的Info
	size = mapInfo.bmiHeader.biSizeImage;							//得知圖片的大小

	image = (unsigned char*) operator new(size*sizeof(unsigned char));//配置記憶體給圖片使用
	fread(image, 1, size, fp);										//讀取圖片

	fclose(fp);

	width = mapInfo.bmiHeader.biWidth;
	height= mapInfo.bmiHeader.biHeight;

	glGenTextures(1,&my_texture_id);
	glBindTexture(GL_TEXTURE_2D, my_texture_id);
	glTexImage2D(	GL_TEXTURE_2D,
						0,
						3,
						width,
						height,
						0,
						GL_BGR_EXT,
						GL_UNSIGNED_BYTE,
						image);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}


