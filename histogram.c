/*
 * histogram.c
 *
 *  Created on: 6.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <string.h>
#include "histogram.h"
#include "common.h"

RETCODE histogram_extract(SDL_Texture *src, Histogram *r, Histogram *g, Histogram *b)
{
	uint32_t format;
	int acc, w, h;

	if(SDL_QueryTexture(src, &format, &acc, &w, &h) != 0) {
		return RC_FAIL;
	}

	if(format != SDL_PIXELFORMAT_RGBA8888) {
		return RC_INVALIDARG;
	}

	uint8_t *pixels;
	int stride;

	if(SDL_LockTexture(src, NULL, (void**)&pixels, &stride) != 0) {
		return RC_FAIL;
	}

	int i, j;
	memset((void*)r->values, 0, 256);
	memset((void*)g->values, 0, 256);
	memset((void*)b->values, 0, 256);

	for(j=0; j<h; j++) {
		uint8_t *pix = pixels + j * stride;

		for(i=0; i<w; i++) {
			r->values[pix[1]]++;
			g->values[pix[2]]++;
			b->values[pix[3]]++;
		}
	}

	SDL_UnlockTexture(src);

	/* Evaluate effective range and average */
	histogram_evaluate_statistics(r);
	histogram_evaluate_statistics(g);
	histogram_evaluate_statistics(b);

	return RC_OK;
}

RETCODE histogram_evaluate_statistics(Histogram *h)
{
	int i;

	h->avg = 0;
	h->max = 255;
	h->min = 0;
	h->maxval = 0;

	int min_flag = 0, max_flag = 0;

	for(i=0; i<256; i++) {
		/* Calculate min/max range */
		if(!min_flag && h->values[i] == 0) {
			h->min = i;
		}
		min_flag = min_flag || h->values[i] > 0;

		if(!max_flag && h->values[255-i] == 0) {
			h->max = 255-i;
		}
		max_flag = max_flag || h->values[255-i] > 0;

		/* Calculate max value */
		if(h->values[i] > h->maxval) {
			h->maxval = h->values[i];
		}

		h->avg += h->values[i];
	}

	h->avg /= 256;
	return RC_OK;
}
