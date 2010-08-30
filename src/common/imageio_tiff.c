/*
    This file is part of darktable,
    copyright (c) 2010 Henrik Andersson.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <memory.h>
#include <stdio.h>
#include <tiffio.h>
#include <inttypes.h>
#include <strings.h>
#include "imageio.h"
#include "imageio_tiff.h"
#include "common/exif.h"
#include "common/colorspaces.h"

dt_imageio_retval_t dt_imageio_open_tiff(dt_image_t *img, const char *filename)
{
  const char *ext = filename + strlen(filename);
  while(*ext != '.' && ext > filename) ext--;
  if(strncmp(ext, ".tif", 4) && strncmp(ext, ".TIF", 4) && strncmp(ext, ".tiff", 5) && strncmp(ext, ".TIFF", 5))
    return DT_IMAGEIO_FILE_CORRUPTED;
  if(!img->exif_inited)
    (void) dt_exif_read(img, filename);

  TIFF *image;
  uint32_t width, height, bpp, imagesize, config;

  if((image = TIFFOpen(filename, "rb")) == NULL) return DT_IMAGEIO_FILE_CORRUPTED;

  TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bpp);
  if(bpp < 12) imagesize =   4*(height * width + 1);
  else         imagesize = 2*4*(height * width + 1);

  img->width = width;
  img->height = height;
  
  if(dt_image_alloc(img, DT_IMAGE_FULL))
  {
    TIFFClose(image);
    return DT_IMAGEIO_CACHE_FULL;
  }
  dt_image_check_buffer(img, DT_IMAGE_FULL, 3*img->width*img->height*sizeof(float));

	uint32_t imagelength;
  int32_t scanlinesize = TIFFScanlineSize(image);
  int32_t mul = scanlinesize/width;
	tdata_t buf;
	buf = _TIFFmalloc(scanlinesize);
  uint16_t *buf16 = (uint16_t *)buf;
  uint8_t *buf8 = (uint8_t *)buf;
	uint32_t row;

	TIFFGetField(image, TIFFTAG_IMAGELENGTH, &imagelength);
	TIFFGetField(image, TIFFTAG_PLANARCONFIG, &config);
  if (config == PLANARCONFIG_CONTIG)
  {
    for (row = 0; row < imagelength; row++)
    {
      TIFFReadScanline(image, buf, row, 0);
      if(bpp < 12) for(int i=0;i<width;i++)
        for(int k=0;k<3;k++) img->pixels[3*(width*row + i) + k] = buf8[mul*i + k]*(1.0/255.0);
      else for(int i=0;i<width;i++)
        for(int k=0;k<3;k++) img->pixels[3*(width*row + i) + k] = buf16[mul/2*i + k]*(1.0/65535.0);
        // for(int k=0;k<3;k++) img->pixels[3*(width*row + i) + k] = ((buf16[mul*i + k]>>8)|((buf16[mul*i + k]<<8)&0xff00))*(1.0/65535.0);
    }
  }
  else if (config == PLANARCONFIG_SEPARATE)
  {
    assert(0);
    // uint16_t s, nsamples;
    // TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
    // for (s = 0; s < nsamples; s++)
    //   for (row = 0; row < imagelength; row++)
    //     TIFFReadScanline(image, buf, row, s);
  }
  _TIFFfree(buf);
  TIFFClose(image);
  dt_image_release(img, DT_IMAGE_FULL, 'w');
  img->flags |= DT_IMAGE_LDR;
  return DT_IMAGEIO_OK;
}

dt_imageio_retval_t dt_imageio_open_tiff_preview(dt_image_t *img, const char *filename)
{
  const char *ext = filename + strlen(filename);
  while(*ext != '.' && ext > filename) ext--;
  if(strncmp(ext, ".tif", 4) && strncmp(ext, ".TIF", 4) && strncmp(ext, ".tiff", 5) && strncmp(ext, ".TIFF", 5))
    return DT_IMAGEIO_FILE_CORRUPTED;
  if(!img->exif_inited)
    (void) dt_exif_read(img, filename);

  TIFF *image;
  uint32_t width, height, bpp, imagesize, config;

  if((image = TIFFOpen(filename, "rb")) == NULL) return DT_IMAGEIO_FILE_CORRUPTED;

  TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bpp);
  if(bpp < 12) imagesize =   4*(height * width + 1);
  else         imagesize = 2*4*(height * width + 1);

  img->width = width;
  img->height = height;
  

	uint32_t imagelength;
  int32_t scanlinesize = TIFFScanlineSize(image);
  int32_t mul = scanlinesize/width;
	tdata_t buf;
	buf = _TIFFmalloc(scanlinesize);
	uint32_t row;
  void *tmp;
  if(bpp < 12) tmp = (void *)malloc(sizeof(uint8_t)*3*width*height);
  else         tmp = (void *)malloc(sizeof(uint16_t)*3*width*height);
  uint16_t *buf16 = (uint16_t *)buf, *tmp16 = (uint16_t *)tmp;
  uint8_t  *buf8  = (uint8_t *)buf,  *tmp8  = (uint8_t *)tmp;

  const int orientation = 0;
	TIFFGetField(image, TIFFTAG_IMAGELENGTH, &imagelength);
	TIFFGetField(image, TIFFTAG_PLANARCONFIG, &config);
  if (config == PLANARCONFIG_CONTIG)
  {
    for (row = 0; row < imagelength; row++)
    {
      TIFFReadScanline(image, buf, row, 0);
      if(bpp < 12) for(int i=0;i<width;i++)
        for(int k=0;k<3;k++) tmp8[3*(width*row + i) + k] = buf8[mul*i + k];
      else for(int i=0;i<width;i++)
        for(int k=0;k<3;k++) tmp16[3*(width*row + i) + k] = buf16[mul/2*i + k];
    }
  }

  if(dt_image_alloc(img, DT_IMAGE_MIP4))
  {
    free(tmp);
    _TIFFfree(buf);
    TIFFClose(image);
    return DT_IMAGEIO_CACHE_FULL;
  }

  int p_wd, p_ht;
  float f_wd, f_ht;
  dt_image_get_mip_size(img, DT_IMAGE_MIP4, &p_wd, &p_ht);
  dt_image_get_exact_mip_size(img, DT_IMAGE_MIP4, &f_wd, &f_ht);

  // printf("mip sizes: %d %d -- %f %f\n", p_wd, p_ht, f_wd, f_ht);
  // FIXME: there is a black border on the left side of a portrait image!

  dt_image_check_buffer(img, DT_IMAGE_MIP4, 4*p_wd*p_ht*sizeof(uint8_t));
  const int p_ht2 = orientation & 4 ? p_wd : p_ht; // pretend unrotated preview, rotate in write_pos
  const int p_wd2 = orientation & 4 ? p_ht : p_wd;
  const int f_ht2 = MIN(p_ht2, (orientation & 4 ? f_wd : f_ht) + 1.0);
  const int f_wd2 = MIN(p_wd2, (orientation & 4 ? f_ht : f_wd) + 1.0);
  if(img->width == p_wd && img->height == p_ht)
  { // use 1:1
    for (int j=0; j < height; j++)
      for (int i=0; i < width; i++)
        if(bpp >= 12)
          for(int k=0;k<3;k++) img->mip[DT_IMAGE_MIP4][4*dt_imageio_write_pos(i, j, p_wd2, p_ht2, f_wd2, f_ht2, orientation)+2-k] = tmp16[3*width*j+3*i+k]>>8;
        else
          for(int k=0;k<3;k++) img->mip[DT_IMAGE_MIP4][4*dt_imageio_write_pos(i, j, p_wd2, p_ht2, f_wd2, f_ht2, orientation)+2-k] = tmp8 [3*width*j+3*i+k];
  }
  else
  { // scale to fit
    bzero(img->mip[DT_IMAGE_MIP4], 4*p_wd*p_ht*sizeof(uint8_t));
    const float scale = fmaxf(img->width/f_wd, img->height/f_ht);
    for(int j=0;j<p_ht2 && scale*j<height;j++) for(int i=0;i<p_wd2 && scale*i < width;i++)
    {
      uint8_t cam[3];
      if(bpp < 12) for(int k=0;k<3;k++) cam[k] = tmp8 [3*((int)(scale*j)*width + (int)(scale*i)) + k];
      else         for(int k=0;k<3;k++) cam[k] = tmp16[3*((int)(scale*j)*width + (int)(scale*i)) + k] >> 8;
      for(int k=0;k<3;k++) img->mip[DT_IMAGE_MIP4][4*dt_imageio_write_pos(i, j, p_wd2, p_ht2, f_wd2, f_ht2, orientation)+2-k] = cam[k];
    }
  }
  free(tmp);
  _TIFFfree(buf);
  TIFFClose(image);

  dt_image_release(img, DT_IMAGE_MIP4, 'w');
  dt_imageio_retval_t retval = dt_image_update_mipmaps(img);
  retval += dt_image_preview_to_raw(img);
  dt_image_release(img, DT_IMAGE_MIP4, 'r');
  img->flags |= DT_IMAGE_LDR;
  return retval;
}






