/*
 * histogram.h
 *
 *  Created on: 6.05.2016 ã.
 *      Author: Anton Angelov
 */

#ifndef HISTOGRAM_H_
#define HISTOGRAM_H_

#include <stdint.h>
#include <SDL2/SDL.h>
#include "common.h"

typedef struct {
	/* Minimum and maximum range */
	int32_t min;
	int32_t max;

	/* Maximum value */
	int32_t maxval;

	/* Average */
	int32_t avg;

	/* Values measured */
	int32_t val_count;

	int32_t values[256];
} Histogram;

RETCODE histogram_extract(SDL_Texture *src, Histogram *r, Histogram *g, Histogram *b);
RETCODE histogram_evaluate_statistics(Histogram *h);

#endif /* HISTOGRAM_H_ */
