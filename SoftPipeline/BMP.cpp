#include "BMP.h"
#include <cstdlib>
#include <cassert>
#include <cstdio>

#define _SIGNATURE 0x4d42
#define _BITS_OF_PIXEL 24
#define _COMPRESSION 0
#define _X_PIXEL_PER_METER 0
#define _Y_PIXEL_PER_METER 0
#define _COLORS_IN_COLOR_TABLE 0
#define _IMPORTANT_COLOR_COUNT 0
#define BYTES_OF_PIXEL 3
#define ALIGN(x,a)    (((x)+(a)-1)&~(a-1))    

namespace BMP {

	Color Color::black(0,0,0);
	Color Color::white(1, 1, 1);
	Color Color::red(1,0,0);
	Color Color::green(0, 1,0);
	Color Color::blue(0,0, 1);

	UINT32 BMP::GetWidth()const
	{
		return _width;
	}

	UINT32 BMP::GetHeight()const
	{
		return _height;
	}

	BMP::BMP()
		:_fileName(), _width(0), _height(0), _buffer(NULL),_rowSize(0)
	{
		assert(sizeof(UINT8) == 1);
		assert(sizeof(UINT16) == 2);
		assert(sizeof(UINT32) == 4);

		assert(sizeof(BITMAPFILEHEADER) == 14);
		assert(sizeof(DIBHEADER) == 40);
	}

	void BMP::SetOutPut(const char * fileName, unsigned width, unsigned height)
	{
		
		this->_fileName = fileName;
		this->_width = width;
		this->_height = height;
		this->_buffer = NULL;
		this->_rowSize = 0;

		assert(_width);
		assert(_height);

		_rowSize = ALIGN(_width, 4);
		UINT32 imageSize = _height * _rowSize * BYTES_OF_PIXEL;// assert the bits of pixel is 24 = 3 * 8
		
		// initialize the DIB header
		_bitmapFileHeader.signature = _SIGNATURE;
		_bitmapFileHeader.fileSize = sizeof(BITMAPFILEHEADER) + sizeof(DIBHEADER) + imageSize;
		_bitmapFileHeader.reserved = 0;
		_bitmapFileHeader.fileOffsetToPixelArray = sizeof(BITMAPFILEHEADER) + sizeof(DIBHEADER);


		// initialize the DIB header
		_dibHeader.dibHeaderSize = sizeof(DIBHEADER);
		_dibHeader.imageWidth = _rowSize;
		_dibHeader.imageHeight = _height;
		_dibHeader.planes = 1;
		_dibHeader.bitsPerPixel = _BITS_OF_PIXEL;
		_dibHeader.compression = _COMPRESSION;
		_dibHeader.imageSize = sizeof(UINT8) * imageSize;
		_dibHeader.xPiexlPerMeter = _X_PIXEL_PER_METER;
		_dibHeader.yPiexlPerMeter = _Y_PIXEL_PER_METER;
		_dibHeader.colorsInColorTable = _COLORS_IN_COLOR_TABLE;
		_dibHeader.importantColorCount = _IMPORTANT_COLOR_COUNT;

		// malloc the memories of buffer
		_buffer = static_cast<UINT8*>(malloc(_dibHeader.imageSize));
	}

	void BMP::ReadFrom(const char * fileName)
	{
		this->_fileName = fileName;

		FILE* f = fopen(fileName, "rb");
	
		fread(&(this->_bitmapFileHeader), sizeof(BITMAPFILEHEADER), 1, f);
		fread(&(this->_dibHeader), sizeof(DIBHEADER), 1, f);

		if (this->_buffer)
			free(this->_buffer);
		this->_buffer = static_cast<UINT8*>(malloc(_dibHeader.imageSize));

		fread(this->_buffer, sizeof(UINT8), _dibHeader.imageSize,f);

		this->_width = _dibHeader.imageWidth;
		this->_height = _dibHeader.imageHeight;
		this->_rowSize = ALIGN(_width, 4);

		assert(_width);
		assert(_height);


		fclose(f);
	}

	BMP::~BMP()
	{
		free(_buffer);
		free(_mipMapBuffer);
	}

	void BMP::drawPixelAt(ColorPass r, ColorPass g, ColorPass b, unsigned x, unsigned y)
	{

		assert(x < _width);
		assert(y < _height);

		_buffer[(y * _rowSize + x) * BYTES_OF_PIXEL] = b;
		_buffer[(y * _rowSize + x) * BYTES_OF_PIXEL + 1] = g;
		_buffer[(y * _rowSize + x) * BYTES_OF_PIXEL + 2] = r;

	}

	void BMP::drawPixelAt(float r, float g, float b, unsigned x, unsigned y)
	{
		r = fmax(0, r);
		r = fmin(1, r);
		b = fmax(0, b);
		b = fmin(1, b);
		g = fmax(0, g);
		g = fmin(1, g);

		drawPixelAt(
			(ColorPass)(r * 255),
			(ColorPass)(g * 255), 
			(ColorPass)(b * 255),
			x, y);
	}

	void BMP::drawPixelAt(const Color & c, unsigned x, unsigned y)
	{
		drawPixelAt(c.r, c.g, c.b, x, y);
	}

	void BMP::GetColorAt(unsigned x, unsigned y, Color* color) const
	{
		assert(x < _width);
		assert(y < _height);


		ColorPass b = _buffer[(y * _rowSize + x) * BYTES_OF_PIXEL];
		ColorPass g = _buffer[(y * _rowSize + x) * BYTES_OF_PIXEL + 1];
		ColorPass r = _buffer[(y * _rowSize + x) * BYTES_OF_PIXEL + 2];
		
		float inv = 1.0f / 255;

		color->r = r * inv;
		color->g = g * inv;
		color->b = b * inv;
	}
	
	void BMP::writeImage(const char* name)
	{
		if (name == NULL)
			name = _fileName.c_str();
		FILE* f = fopen(name, "wb");
		assert(f);

		fwrite(&_bitmapFileHeader, sizeof(_bitmapFileHeader),1,f);
		fwrite(&_dibHeader, sizeof(_dibHeader), 1, f);
		fwrite(_buffer, _dibHeader.imageSize, 1, f);

		fclose(f);
	}

	void BMP::WriteMipMap(int offset, int sourceOffset, int srcWidth, int resolutionX, int resolutionY)
	{
		int last = offset + resolutionX * resolutionY;

		int r, g, b;
		
		int tsOffset;
		int csRowOffsetStart = sourceOffset;
		int csOffset = sourceOffset;

		for (;offset < last; offset++)
		{
			r = g = b = 0;

			tsOffset = csOffset;
			b += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL];
			g += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 1];
			r += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 2];

			tsOffset = csOffset + 1;
			b += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL];
			g += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 1];
			r += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 2];

			tsOffset = csOffset + srcWidth;
			b += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL];
			g += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 1];
			r += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 2];

			tsOffset = tsOffset + 1;
			b += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL];
			g += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 1];
			r += _mipMapBuffer[(tsOffset) * BYTES_OF_PIXEL + 2];

			_mipMapBuffer[(offset) * BYTES_OF_PIXEL] = b * 0.25f;
			_mipMapBuffer[(offset) * BYTES_OF_PIXEL + 1] = g * 0.25f;
			_mipMapBuffer[(offset) * BYTES_OF_PIXEL + 2] = r * 0.25f;

			csOffset += 2;
			if (csOffset - csRowOffsetStart >= (srcWidth & 0xfffffffe))
			{
				csRowOffsetStart += 2 * srcWidth;
				csOffset = csRowOffsetStart;
			}
		}
	}

	void BMP::GetMipmapData(int mipmapLevel, int& offset, int& rowSize) const
	{
		int curResolutionX = _width;
		int curResolutionY = _height;

		offset = 0;

		int curLevel = 0;

		while (curResolutionX > 1 && curResolutionY > 1)
		{
			if (mipmapLevel == curLevel)
			{
				break;
			}

			offset += curResolutionX * curResolutionY;

			curResolutionX = curResolutionX / 2;
			curResolutionY = curResolutionY / 2;

			curLevel += 1;
		}

		rowSize = curResolutionX;
	}

	void BMP::GenerateMipMap()
	{
		if (_mipMapBuffer != nullptr)
		{
			free(_mipMapBuffer);
		}

		int total = (_width * _height);

		_mipMapBuffer = static_cast<UINT8*>(malloc(BYTES_OF_PIXEL * (total + total / 2)));

		memcpy(_mipMapBuffer, _buffer, total * BYTES_OF_PIXEL);


		int curResolutionX = _width / 2;
		int curResolutionY = _height / 2;

		int srcOffset = 0;
		int offset = total;

		int oldWidth = _width;

		while (curResolutionX > 0 && curResolutionY > 0)
		{
			WriteMipMap(offset, srcOffset, oldWidth, curResolutionX, curResolutionY);

			srcOffset = offset;
			offset += curResolutionX * curResolutionY;

			curResolutionX = (curResolutionX / 2);
			curResolutionY = (curResolutionY / 2);
			
			oldWidth = (oldWidth / 2);
		}

	}

	void BMP::writeMipMapImage(const char * name)
	{
		BMP mipMap;
		
		UINT32 mipmapWidth = _width + _width / 2;

		mipMap.SetOutPut(name, mipmapWidth, _height);


		for (int i = 0; i < _width; i++)
		{
			for (int j = 0; j < _height; j++)
			{
				mipMap.drawPixelAt(GetColorAt(i,j,0), i, j);
			}
		}

		UINT32 cWidth = _width / 2;
		UINT32 sy = 0;
		UINT32 cHeight = _height / 2;
		UINT32 mipMapLevel = 1;

		while (cWidth > 0)
		{
			for (int i = 0; i < cWidth; i++)
			{
				for (int j = 0; j < cHeight; j++)
				{
					mipMap.drawPixelAt(GetColorAt(i, j, mipMapLevel), i + _width, j + sy);
				}
			}

			cWidth /= 2;
			sy += cHeight;
			cHeight /= 2;
			mipMapLevel += 1;
		}

		mipMap.writeImage();
	}

	Color BMP::GetColorAt(unsigned x, unsigned y, int mipmapLevel)
	{
		Color res;

		int offset, rowSize;
		GetMipmapData(mipmapLevel,offset, rowSize);

		ColorPass b = _mipMapBuffer[(offset + (y * rowSize + x)) * BYTES_OF_PIXEL];
		ColorPass g = _mipMapBuffer[(offset + (y * rowSize + x)) * BYTES_OF_PIXEL + 1];
		ColorPass r = _mipMapBuffer[(offset + (y * rowSize + x)) * BYTES_OF_PIXEL + 2];

		float inv = 1.0f / 255;

		res.r = r * inv;
		res.g = g * inv;
		res.b = b * inv;

		return res;
	}

	Color::Color():Color(0,0,0)
	{
	}

	Color::Color(float r, float g, float b) : r(r), g(g), b(b)
	{
	}


	Color::Color(const Color & color):Color(color.r, color.g, color.b)
	{
		
	}

	Color Color::add(const Color & color)const
	{
		return Color(r + color.r,g + color.g,b + color.b);
	}

	Color Color::modulate(const Color & color)const
	{
		return Color(r * color.r, g * color.g, b * color.b);
	}

	Color Color::multiply(float s)const
	{
		return Color(r * s, g * s, b * s);
	}
}