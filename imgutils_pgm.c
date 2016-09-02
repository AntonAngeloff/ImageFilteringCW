/*
 * imgutils_pgm.c
 *
 *  Created on: 5.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "imgutils.h"

RETCODE pgm_test(FILE *f, int *format, int *w, int *h)
{
	char identifier[1024];
	char description[1024];

	long int pos = ftell(f);
	int width, height;
	RETCODE rc = RC_OK;

	/* Read identifier */
	fscanf(f, "%2s\n", identifier);
	if(strcmp(identifier, "P2") != 0) {
		/* Invalid identifier */
		rc = RC_INVALIDDATA;
		goto end;
	}

	/* Read description */
	fgets(description, 1024, f);
	if(description[0] != '#') {
		/* Description should start with '#' */
		rc = RC_INVALIDDATA;
		goto end;
	}

	/* Read dimensions */
	if (fscanf(f, "%1024d %d", &width, &height) <= 0) {
		rc = RC_INVALIDDATA;
		goto end;
	}

	if(w) *w = width;
	if(h) *h = height;
	if(format) *format = SDL_PIXELFORMAT_RGBA8888;

end:
	fseek(f, pos, SEEK_SET);

	/* Seems like a valid file */
	return rc;
}

RETCODE pgm_load(FILE *f, SDL_Texture *target)
{
	char identifier[1024];
	char description[1024];
	int32_t width, height;
	int32_t maxval;

	/* Read identifier */
	fscanf(f, "%s\n", identifier);
	if(strcmp(identifier, "P2") != 0) {
		/* Invalid identifier */
		return RC_INVALIDDATA;
	}

	/* Read description */
	fgets(description, 1024, f);
	if(description[0] != '#') {
		/* Description should start with '#' */
		return RC_INVALIDDATA;
	}

	/* Read dimensions */
	if (fscanf(f, "%d %d", &width, &height) <= 0) {
		return RC_INVALIDDATA;
	}

	/* Read maximum value of grey */
	if (fscanf(f, "%d", &maxval) <= 0) {
		return RC_INVALIDDATA;
	}

	/* Make sure texture has same size and format as the image in the file */
	uint32_t format;
	int access, w, h;

	if (SDL_QueryTexture(target, &format, &access, &w, &h) != 0) {
		return RC_FAIL;
	}

	if(w!=width || h!=height || format!=SDL_PIXELFORMAT_RGBA8888) {
		return RC_INVALIDARG;
	}

	/* Lock texture data and update it's content according to the content in the file */
	uint8_t *pix_data;
	int stride;

	if (SDL_LockTexture(target, NULL, (void**)&pix_data, &stride) != 0) {
		printf("SDL_LockTexture failed (%s).\n", SDL_GetError());
		return RC_FAIL;
	}

	int i, j, value;

	/* Load pixel data from file */
	for(j=0; j<h; j++) {
		uint8_t *ptr = pix_data + stride * j;

		for(i=0; i<w; i++) {
			if(fscanf(f, "%d", &value) <= 0) {
				break;
			}

			/* Rescale value to [0..254] */
			value = value * 255 / maxval;

			/* Populate R/G/B/A components for current pixel */
			*(ptr++) = 255;
			*(ptr++) = (uint8_t)value;
			*(ptr++) = (uint8_t)value;
			*(ptr++) = (uint8_t)value;
		}
	}

	/* Unlock texture */
	SDL_UnlockTexture(target);

	return RC_OK;
}

RETCODE pgm_save(FILE *f, SDL_Texture *source)
{
	uint32_t format;
	int acc, w, h;

	if(SDL_QueryTexture(source, &format, &acc, &w, &h) != 0) {
		return RC_FAIL;
	}

	/* Write signature */
	fprintf(f, "P2\n");

	/* Write description */
	fprintf(f, "# Created by course work project\n");

	/* Write dimentions and max value */
	fprintf(f, "%d %d\n%d\n", w, h, 255);

	uint8_t *pixels;
	int32_t stride;

	if(SDL_LockTexture(source, NULL, (void**)&pixels, &stride) != 0) {
		return RC_FAIL;
	}

	int32_t i, j;

	/* Write values */
	for(j=0; j<h; j++) {
		uint8_t *line = pixels + stride * j;

		for(i=0; i<w; i++) {
			line++;

			int32_t gray = (int32_t)line[0] + (int32_t)line[1] + (int32_t)line[2];
			gray /= 3;
			line += 3;

			fprintf(f, "%d ", gray);
		}

		fprintf(f, "\n");
	}

	/* Unlock */
	SDL_UnlockTexture(source);

	return RC_OK;
}

/* Define a structure describing the image handler */
IMGHandler imgutils_pgm_handler = {
	.format_name = "pgm",
	.is_bin = 0,
	.image_test = pgm_test,
	.image_load = pgm_load,
	.image_save = pgm_save,
};
