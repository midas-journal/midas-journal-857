#include "image.h"
#include <png.h>

static void readDataFromMemory(png_structp png_ptr,
                               png_bytep outBytes,
                               png_size_t byteCountToRead) {
  char *buffer = (char*)png_ptr->io_ptr;
  memcpy(outBytes, buffer, byteCountToRead);
  png_ptr->io_ptr = buffer+byteCountToRead;
}

void Image::readPNG(void *imageData, size_t dataSize)
{
  char *buffer = (char*)imageData;
  if(!png_check_sig((png_bytep)buffer, 8))
    abort();

  buffer += 8;
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);

  png_set_read_fn(png_ptr, buffer, readDataFromMemory);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);
  
  int components = 0;
  if (info_ptr->color_type & PNG_COLOR_MASK_COLOR)
    components += 3;
  if (info_ptr->color_type == PNG_COLOR_MASK_ALPHA)
    components += 1;

  this->hostAllocate(info_ptr->width, info_ptr->height);
  
  png_byte *buf = (png_byte*)malloc(info_ptr->rowbytes);
  switch (info_ptr->bit_depth)
    {
    case 8:
      for (int y=0; y<this->height; y++) {
        png_read_row(png_ptr, buf, NULL);
        for (int x=0; x<this->width; x++)
          this->data[y*this->width+x] = buf[x*components+0] +
            (buf[x*components+1]<<8) + (buf[x*components+2]<<16);
      }
      break;
    case 16:
      for (int y=0; y<this->height; y++) {
        png_read_row(png_ptr, buf, NULL);
        uint16_t *ubuf = (uint16_t*)buf;
        for (int x=0; x<this->width; x++)
          this->data[y*this->width+x] = ((uint32_t)ubuf[x*components+2] >> 8) +
            (((uint32_t)ubuf[x*components+1] >> 8)<<8) +
            (((uint32_t)ubuf[x*components+0] >> 8)<<16);
      }
      break;
    default:
      fprintf(stderr, "UNHANDLED BIT DEPTH %d\n", info_ptr->bit_depth);
    }
  free(buf);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}
