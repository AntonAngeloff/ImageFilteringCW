/*
 * imgutils.h
 *
 *  Created on: 5.05.2016 ã.
 *      Author: Anton Angelov
 */

#ifndef IMGUTILS_H_
#define IMGUTILS_H_

#include <SDL2/SDL.h>
#include "common.h"

typedef struct {
	char *format_name;

	/* If the format is binary or text */
	int	is_bin;

	/**
	 * Function for testing a image if it's handled by this current image handler.
	 * This function is responsible not to modify the FILE position pointer.
	 */
	RETCODE (*image_test)(FILE *f, int *format, int *w, int *h);

	/**
	 * Function for loading a image from FILE and uploads it to already existing SDL texture.
	 * If the source pixel format doesn't match with the texture format, the function will try to convert the image.
	 */
	RETCODE (*image_load)(FILE *f, SDL_Texture *target);

	/**
	 * Function for saving contents of a SDL texture into a file.
	 */
	RETCODE (*image_save)(FILE *f, SDL_Texture *source);
} IMGHandler;

RETCODE image_get_info(FILE *f, int32_t *format, int32_t *w, int32_t *h);
RETCODE image_load(FILE *f, SDL_Texture *target);
RETCODE image_load_from_file(char *filename, SDL_Texture *target);
RETCODE image_save(FILE *f, char *format_name, SDL_Texture *source);
RETCODE image_save_to_file(char *fn, char *format_name, SDL_Texture *source);

#endif /* IMGUTILS_H_ */
