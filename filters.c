/*
 * filters.c
 *
 *  Created on: 6.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <malloc.h>
#include <string.h>
#include "filters.h"

#define CLAMP(x, a, b) if(x < a) x = a; else if (x > b) x = b;
#define bytes_per_pixel 4

Filter2D *filter_list;
int filter_count;

RETCODE filter_apply(void *src, void *dst, int stride, int w, int h, Filter2D *filter)
{
	int i, j, x, y;

	/* We don't support filters with even dimensions */
	if(filter->w % 2 == 0 || filter->h % 2 == 0) {
		return RC_FAIL;
	}

	/* Calculate filter's half width and half height */
	int filter_hw = filter->w / 2;
	int filter_hh = filter->h / 2;

	for(j=0; j<h; j++) {
		uint8_t *d_line = dst + stride * j;

		for(i=0; i<w; i++) {
			int32_t product[bytes_per_pixel] = {0, 0, 0, 1};

			/* Apply the convulation matrix and store the result in product[] */
			for(y=-filter_hh; y<=filter_hh; y++) {
				/* Calculate source pixel's scanline, by wrapping the overbound indices */
				uint8_t *fy_src = src + (((j + y) + h) % h) * stride;

				for(x=-filter_hw; x<=filter_hw; x++) {
					/* Calculate source pixel's address */
					uint8_t *fx_src = fy_src + (((x + i) + w) % w) * bytes_per_pixel;

					int filter_x = x + filter_hw;
					int filter_y = y + filter_hh;
					int filter_element_id = filter_y * filter->w + filter_x;

					/* Perform operation on R/G/B components and leave alpha value untouched */
					product[0] += fx_src[1] * filter->matrix[filter_element_id];
					product[1] += fx_src[2] * filter->matrix[filter_element_id];
					product[2] += fx_src[3] * filter->matrix[filter_element_id];
				}
			}

			/* Divide the product by the common divisor */
			product[0] /= filter->divisor;
			product[1] /= filter->divisor;
			product[2] /= filter->divisor;

			CLAMP(product[0], 0, 255);
			CLAMP(product[1], 0, 255);
			CLAMP(product[2], 0, 255);

			/* After the matrix is applied, write the produced pixel onto the destination
			 * bitmap
			 */
			uint8_t *d_pixel = d_line + i * bytes_per_pixel;
			d_pixel[1] = (uint8_t)product[0];
			d_pixel[2] = (uint8_t)product[1];
			d_pixel[3] = (uint8_t)product[2];
		}
	}

	/* Success */
	return RC_OK;
}

RETCODE filter_find_by_name(const char *name, Filter2D **out)
{
	int i;

	for(i=0; i<filter_count; i++) {
		if(strcmp(filter_list[i].name, name) != 0)
			continue;

		*out = &filter_list[i];
		return RC_OK;
	}

	return RC_FAIL;
}

RETCODE filter_find_by_id(const int id, Filter2D **out)
{
	if(id < 0 || id >= filter_count) {
		return RC_FAIL;
	}

	*out = &filter_list[id];
	return RC_OK;
}

RETCODE filter_apply_to_texture(SDL_Texture *src, SDL_Texture *dst, const char *filter_name)
{
	RETCODE rc;
	Filter2D *filter;

	/* Find the filter */
	rc = filter_find_by_name(filter_name, &filter);
	if(failed(rc)) return rc;

	if(!src || !dst || !filter) {
		return RC_INVALIDARG;
	}
	void *src_pixels, *dst_pixels;
	int src_stride, dst_stride;

	if(SDL_LockTexture(src, NULL, &src_pixels, &src_stride) != 0) {
		return RC_FAIL;
	}

	if(SDL_LockTexture(dst, NULL, &dst_pixels, &dst_stride) != 0) {
		rc = RC_FAIL;
		goto unlock;
	}

	uint32_t format;
	int access, width, height;
	if(SDL_QueryTexture(src, &format, &access, &width, &height) != 0) {
		rc = RC_FAIL;
		goto unlock;
	}

	rc = filter_apply(src_pixels, dst_pixels, src_stride, width, height, filter);

unlock:
	SDL_UnlockTexture(src);
	SDL_UnlockTexture(dst);

	return rc;
}

RETCODE copy_texture(SDL_Texture *src, SDL_Texture *dst)
{
	void *src_pixels, *dst_pixels;
	int src_stride, dst_stride;
	RETCODE rc;

	if(SDL_LockTexture(src, NULL, &src_pixels, &src_stride) != 0) {
		return RC_FAIL;
	}

	if(SDL_LockTexture(dst, NULL, &dst_pixels, &dst_stride) != 0) {
		rc = RC_FAIL;
		goto unlock;
	}

	uint32_t format;
	int access, width, height;
	if(SDL_QueryTexture(src, &format, &access, &width, &height) != 0) {
		rc = RC_FAIL;
		goto unlock;
	}

	if(format != SDL_PIXELFORMAT_RGBA8888) {
		return RC_INVALIDARG;
	}

	memcpy(dst_pixels, src_pixels, height * width * 4);

unlock:
	SDL_UnlockTexture(src);
	SDL_UnlockTexture(dst);

	return rc;
}

/* Blur with 3x3 kernel */
float blur33_kernel[3*3] = {
		0,	1,	0,
		1,	1,	1,
		0,	1,	0,
};

static const Filter2D blur33 = {
		.name = "blur3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 5,
		.matrix = blur33_kernel,
};

/* Blur with 5x5 kernel */
float blur55_kernel[5*5] = {
		  0, 0, 1, 0, 0,
		  0, 1, 1, 1, 0,
		  1, 1, 1, 1, 1,
		  0, 1, 1, 1, 0,
		  0, 0, 1, 0, 0,
};

static const Filter2D blur55 = {
		.name = "blur5x5",
		.w	= 5,
		.h	= 5,
		.divisor = 13,
		.matrix = blur55_kernel,
};

/* Blur with 7x7 kernel */
float blur77_kernel[7*7] = {
		  0, 0, 0, 1, 0, 0, 0,
		  0, 0, 1, 1, 1, 0, 0,
		  0, 1, 1, 1, 1, 1, 0,
		  1, 1, 1, 1, 1, 1, 1,
		  0, 1, 1, 1, 1, 1, 0,
		  0, 0, 1, 1, 1, 0, 0,
		  0, 0, 0, 1, 0, 0, 0,
};

static const Filter2D blur77 = {
		.name = "blur7x7",
		.w	= 5,
		.h	= 5,
		.divisor = 13,
		.matrix = blur77_kernel,
};

/* Gaussian blur with 3x3 kernel */
float gausblur33_kernel[3*3] = {
		1,	2,	1,
		2,	4,	2,
		1,	2,	1,
};

static const Filter2D gausblur33 = {
		.name = "gausblur3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 16,
		.matrix = gausblur33_kernel,
};

/* Edge detection */
float edge33_kernel[3*3] = {
		-1,	-1,	-1,
		-1,	 8,	-1,
		-1,	-1,	-1,
};

static const Filter2D edge33 = {
		.name = "edge3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 1,
		.matrix = edge33_kernel,
};

/* Sharpen filter with 5x5 kernel */
float sharpen33_kernel[3*3] = {
		-0,	-1,	-0,
		-1,	 5,	-1,
		-0,	-1,	-0,
};

static const Filter2D sharpen33 = {
		.name = "sharpen3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 1,
		.matrix = sharpen33_kernel,
};

/* Emboss filter */
float emboss33_kernel[3*3] = {
		-1,	-1,	 0,
		-1,	 0,	 1,
		 0,	 1,	 1,
};

static const Filter2D emboss33 = {
		.name = "emboss3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 1,
		.matrix = emboss33_kernel,
};

/* Sobel horizontal */
float sobel_h33_kernel[3*3] = {
		-1,	 0,	 1,
		-2,	 0,	 2,
		-1,	 0,	 1,
};

static const Filter2D sobel_h33 = {
		.name = "sobel_h3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 1,
		.matrix = sobel_h33_kernel,
};

/* Sobel vertical */
float sobel_v33_kernel[3*3] = {
		-1,	-2,  -1,
		 0,	 0,	  0,
		 1,	 2,	  1,
};

static const Filter2D sobel_v33 = {
		.name = "sobel_v3x3",
		.w	= 3,
		.h	= 3,
		.divisor = 1,
		.matrix = sobel_v33_kernel,
};

void __attribute__((constructor)) filter_init()
{
	filter_list = calloc(10, sizeof(Filter2D));
	filter_count = 0;

	filter_list[filter_count++] = blur33;
	filter_list[filter_count++] = blur55;
	filter_list[filter_count++] = blur77;
	filter_list[filter_count++] = gausblur33;
	filter_list[filter_count++] = edge33;
	filter_list[filter_count++] = sharpen33;
	filter_list[filter_count++] = emboss33;
	filter_list[filter_count++] = sobel_h33;
	filter_list[filter_count++] = sobel_v33;
}

void __attribute__((destructor)) filter_uninit()
{
	free(filter_list);
}
