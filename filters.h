/*
 * filters.h
 *
 *  Created on: 6.05.2016 ã.
 *      Author: Anton Angelov
 */

#ifndef FILTERS_H_
#define FILTERS_H_

#include <stdint.h>
#include <SDL2/SDL.h>
#include "common.h"

typedef struct {
	/* Name of the filter */
	char *name;

	/* Size of the convolution matrix */
	int32_t w;
	int32_t h;

	/* Common divisor */
	float divisor;

	/* Convolution matrix */
	float *matrix;
} Filter2D;

RETCODE filter_find_by_name(const char *name, Filter2D **out);
RETCODE filter_find_by_id(const int id, Filter2D **out);
RETCODE filter_apply(void *src, void *dst, int stride, int w, int h, Filter2D *filter);
RETCODE filter_apply_to_texture(SDL_Texture *src, SDL_Texture *dst, const char *filter_name);
RETCODE copy_texture(SDL_Texture *src, SDL_Texture *dst);

#endif /* FILTERS_H_ */
