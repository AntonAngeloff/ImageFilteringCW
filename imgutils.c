/*
 * imgutils.c
 *
 *  Created on: 5.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "common.h"
#include "imgutils.h"

static IMGHandler *img_handler_arr;
static int img_handler_len;

void __attribute__((constructor)) imgutils_init(void)
{
	img_handler_arr = malloc(100 * sizeof(IMGHandler));
	img_handler_len = 0;

	/* Register PGM image handler */
	extern IMGHandler imgutils_pgm_handler;
	img_handler_arr[img_handler_len++] = imgutils_pgm_handler;

	/* Register BMP handler */
	extern IMGHandler imgutils_bmp_handler;
	img_handler_arr[img_handler_len++] = imgutils_bmp_handler;
}

void __attribute__((destructor)) imgutils_finalize(void)
{
	free(img_handler_arr);
}

RETCODE image_get_info(FILE *f, int32_t *format, int32_t *w, int32_t *h)
{
	int i;
	RETCODE rc;

	for(i=0; i<img_handler_len; i++) {
		rc = img_handler_arr[i].image_test(f, format, w, h);
		if(succeeded(rc)) return rc;
	}

	return RC_FAIL;
}

RETCODE image_load(FILE *f, SDL_Texture *target)
{
	int i;
	RETCODE rc;

	for(i=0; i<img_handler_len; i++) {
		/* Try if the current handler is able to handle this image file */
		rc = img_handler_arr[i].image_test(f, NULL, NULL, NULL);

		if(succeeded(rc)) {
			/* Since the handler has accepted the file, now try to load it */
			rc = img_handler_arr[i].image_load(f, target);
			if(failed(rc)) continue;

			return rc;
		}
	}

	return RC_FAIL;
}

RETCODE image_load_from_file(char *filename, SDL_Texture *target)
{
	FILE *f;

	f = fopen(filename, "r");
	if(!f) {
		/* Failed to open file */
		return RC_FAIL;
	}

	RETCODE rc = image_load(f, target);
	fclose(f);

	return rc;
}

RETCODE image_save(FILE *f, char *format_name, SDL_Texture *source)
{
	int i;

	for(i=0; i<img_handler_len; i++) {
		if(stricmp(format_name, img_handler_arr[i].format_name) != 0)
			continue;

		return img_handler_arr[i].image_save(f, source);
	}

	/* Format not found */
	return RC_FAIL;
}

RETCODE image_save_to_file(char *fn, char *format_name, SDL_Texture *source)
{
	int i;
	char *fm = "w";

	for(i=0; i<img_handler_len; i++) {
		if(stricmp(format_name, img_handler_arr[i].format_name) != 0)
			continue;

		if(img_handler_arr[i].is_bin) {
			fm = "wb";
		}
	}

	FILE *f;

	f = fopen(fn, fm);
	if(!f) {
		/* Failed to open file */
		return RC_FAIL;
	}

	RETCODE rc = image_save(f, format_name, source);
	fclose(f);

	return rc;
}
