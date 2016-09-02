/*
 * imgutils_bmp.c
 *
 *  Created on: 7.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "common.h"
#include "imgutils.h"

typedef struct {
  uint8_t Magic[2];
  uint32_t bfSize;
  uint16_t  bfReserved1;
  uint16_t  bfReserved2;
  uint32_t bfOffBits;
} __attribute__((aligned(1),packed)) BitmapFileHeader;

typedef struct {
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t  biPlanes;
	uint16_t  biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} __attribute__((aligned(1),packed)) BitmapInfoHeader;

RETCODE bmp_test(FILE *f, int *format, int *w, int *h)
{
	int pos = ftell(f);
	RETCODE rc = RC_OK;
	BitmapFileHeader bmfh;
	BitmapInfoHeader bmih;

	/* Read bitmap file header */
	if(fread(&bmfh, sizeof(bmfh), 1, f) != 1) {
		rc = RC_INVALIDDATA;
		goto end;
	}

	if(bmfh.Magic[0] != 'B' || bmfh.Magic[1] != 'M') {
		rc = RC_INVALIDDATA;
		goto end;
	}

	fseek(f, pos + 14, SEEK_SET);

	/* Read bitmap info header */
	if(fread(&bmih, sizeof(bmih), 1, f) != 1) {
		rc = RC_INVALIDDATA;
		goto end;
	}

	if(bmih.biSize != sizeof(bmih)) {
		/* Uncommon bitmap type */
		rc = RC_INVALIDDATA;
		goto end;
	}

	/* We'll handle only 24 and 32 bpp */
	if(bmih.biBitCount!=24 && bmih.biBitCount!=32) {
		rc = RC_INVALIDDATA;
		goto end;
	}

	/* We don't support RLE and other compressions */
	if(bmih.biCompression != 0) {
		rc = RC_INVALIDDATA;
		goto end;
	}

	if(w) *w = bmih.biWidth;
	if(h) *h = bmih.biHeight;
	if(format) *format = SDL_PIXELFORMAT_RGBA8888;

end:
	/* Return stream to original position */
	fseek(f, pos, SEEK_SET);

	return rc;
}

RETCODE bmp_load(FILE *f, SDL_Texture *target)
{
	BitmapFileHeader bmfh;
	BitmapInfoHeader bmih;
	int pos = ftell(f);

	/* Read bitmap file header */
	if(fread(&bmfh, sizeof(bmfh), 1, f) != 1) {
		return RC_INVALIDDATA;
	}

	if(bmfh.Magic[0] != 'B' || bmfh.Magic[1] != 'M') {
		return RC_INVALIDDATA;
	}

	fseek(f, pos + 14, SEEK_SET);

	/* Read bitmap info header */
	if(fread(&bmih, sizeof(bmih), 1, f) != 1) {
		return RC_INVALIDDATA;
	}

	if(bmih.biSize != sizeof(bmih)) {
		/* Uncommon bitmap type */
		return RC_INVALIDDATA;
	}

	/* We'll handle only 24 and 32 bpp */
	if(bmih.biBitCount!=24 && bmih.biBitCount!=32) {
		return RC_INVALIDDATA;
	}

	//fseek(f, pos + bmfh.bfOffBits, SEEK_SET);

	uint8_t *dst;
	int32_t dst_stride, src_stride = (bmih.biBitCount * bmih.biWidth + 31) / 32 * 4;

	/* Lock texture */
	if(SDL_LockTexture(target, NULL, (void**)&dst, &dst_stride) != 0) {
		return RC_FAIL;
	}

	int i, j, height = abs(bmih.biHeight);
	uint8_t *temp = malloc(src_stride);

	for(j=0; j<height; j++) {
		uint8_t *dst_line = dst + ((bmih.biHeight-1) - j) * dst_stride;
		uint8_t *t = temp;

		if(fread(temp, src_stride, 1, f) != 1) {
			goto unlock;
		}

		for(i=0; i<bmih.biWidth; i++) {
			switch(bmih.biBitCount) {
			case 24:
				dst_line[0] = 1;
				dst_line[1] = t[0];
				dst_line[2] = t[1];
				dst_line[3] = t[2];

				dst_line += 4;
				t += 3;
				break;

			case 32:
				*(dst_line++) = *(t++);
				*(dst_line++) = *(t++);
				*(dst_line++) = *(t++);
				*(dst_line++) = *(t++);
				break;
			}
		}
	}

unlock:
	SDL_UnlockTexture(target);
	free(temp);

	return RC_OK;
}

RETCODE bmp_save(FILE *f, SDL_Texture *source)
{
	return RC_NOTIMPL;
}

IMGHandler imgutils_bmp_handler = {
	.format_name = "bmp",
	.is_bin = 1,
	.image_test = bmp_test,
	.image_load = bmp_load,
	.image_save = bmp_save,
};
