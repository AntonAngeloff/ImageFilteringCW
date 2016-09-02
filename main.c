/*
 * main.c
 *
 *  Created on: 4.05.2016 ã.
 *      Author: Anton Angelov
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "common.h"
#include "imgutils.h"
#include "filters.h"
#include "histogram.h"

/* Zoom will be performed in 10 ticks (1/6 second) */
#define ZOOM_SPEED	10

typedef struct {
	SDL_Window *wnd;
	SDL_Renderer *renderer;

	SDL_Texture *orig_image;
	SDL_Texture *filtered_image;

	/* If a filter is applied, then filtered_image will be displayed instead of orig_image */
	int8_t is_filter_applied;

	/* Size of the renderable area of the window */
	SDL_Point renderer_size;

	/* Used for zooming the image */
	float zoom_factor;
	float zoom_animation_progress;
	float zoom_animation_amount;

	/* Histogram for R/G/B components */
	Histogram histograms[3];

	/* Flag signifying if the histograms are shown */
	int show_histograms;

	/* Show both images (original and filtered) */
	int dual_view;
} SDLContext;

/* Quadratic easing creates smoother animation */
float quadratic_easing(float t, float b, float c, float d)
{
	t /= d/2;
	if (t < 1) return c/2*t*t + b;
	t--;
	return -c/2 * (t*(t-2) - 1) + b;
}

/**
 * Destroys the textures in the application's context, if they are
 * allocated at all.
 */
RETCODE sdl_ctx_dispose_textures(SDLContext *ctx)
{
	if(ctx->orig_image != NULL) {
		SDL_DestroyTexture(ctx->orig_image);
		ctx->orig_image = NULL;
	}

	if(ctx->filtered_image != NULL) {
		SDL_DestroyTexture(ctx->filtered_image);
		ctx->filtered_image = NULL;
	}

	return RC_OK;
}

/**
 * Disposes all the resources inside the application context
 * and the context struct variable.
 */
RETCODE sdl_ctx_dispose(SDLContext **ctx)
{
	SDLContext *c = *ctx;

	if(*ctx == NULL) {
		return RC_INVALIDARG;
	}

	/* Dispose textures */
	sdl_ctx_dispose_textures(c);

	/* Destroy renderer */
	if(c->renderer != NULL) {
		SDL_DestroyRenderer(c->renderer);
	}

	/* Destroy window */
	if(c->wnd != NULL) {
		SDL_DestroyWindow(c->wnd);
	}

	free(*ctx);
	(*ctx) = NULL;

	return RC_OK;
}

/**
 * Allocates a SDLContext struct and it's related resources such as
 * window, renderer, etc.
 */
RETCODE sdl_ctx_init(SDLContext **ctx, SDL_Rect wnd_rect, SDL_Point tex_size)
{
	SDLContext *c = *ctx;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

	/* Allocate SDLContext structure */
	if(c == NULL) {
		c = calloc(1, sizeof(SDLContext));
		if(c == NULL) {
			return RC_OUTOFMEM;
		}
	}

	/* Create window */
	if(c->wnd == NULL) {
		c->wnd = SDL_CreateWindow("Course work on Digital Image Processing", wnd_rect.x, wnd_rect.y, wnd_rect.w, wnd_rect.h, SDL_WINDOW_SHOWN);

		/* Check result */
		if(c->wnd == NULL) {
			goto fail;
		}
	}

	/* Create renderer */
	if(c->renderer == NULL) {
		c->renderer = SDL_CreateRenderer(c->wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		/* Check result */
		if (c->renderer == NULL){
			goto fail;
		}
	}

	SDL_GetRendererOutputSize(c->renderer, &c->renderer_size.x, &c->renderer_size.y);
	c->zoom_factor = 1;
	c->zoom_animation_progress = 0;
	c->zoom_animation_amount = 0;
	c->show_histograms = 1;
	c->dual_view = 0;

	/* Success */
	*ctx = c;
	return RC_OK;

fail:
	/* Fail */
	sdl_ctx_dispose(&c);
	return RC_FAIL;
}

RETCODE sdl_ctx_alloc_textures(SDLContext *ctx, char *image_filename)
{
	/* Get file extension */
	char *ext, *fm = "r";

	if((ext = strrchr(image_filename, '.')) != NULL) {
		ext++;

		/* BMP files should be opened as binary */
		if(stricmp(ext, "bmp") == 0) {
			fm = "rb";
		}

		/* Other (PGM) files should be opened as text files */
	}

	FILE *f = fopen(image_filename, fm);
	if(!f) return RC_FAIL;

	/* Destroy old textures, if any */
	sdl_ctx_dispose_textures(ctx);

	int32_t fmt, w, h;
	RETCODE rc = image_get_info(f, &fmt, &w, &h);
	if(failed(rc)) return rc;

	/* Create texture for original image */
	if(ctx) {
		/* Since SDL doesn't support 8bit single channel format, we will convert gray-scale image to RGB32 */
		ctx->orig_image = SDL_CreateTexture(ctx->renderer, fmt, SDL_TEXTUREACCESS_STREAMING, w, h);

		/* Check result */
		if (ctx->orig_image == NULL){
			goto fail;
		}
	}

	/* Load image from file */
	rc = image_load(f, ctx->orig_image);
	if(failed(rc)) return rc;

	/* Create texture for the filtered image */
	if(ctx) {
		/* Since SDL doesn't support 8bit single channel format, we will convert gray-scale image to RGB32 */
		ctx->filtered_image = SDL_CreateTexture(ctx->renderer, fmt, SDL_TEXTUREACCESS_STREAMING, w, h);

		/* Check result */
		if (ctx->filtered_image == NULL){
			goto fail;
		}
	}

	/* Close the image file handle */
	fclose(f);

	/* Success */
	return RC_OK;

fail:
	fclose(f);
	return RC_FAIL;
}

RETCODE draw_histogram(SDLContext *ctx, SDL_Rect bounds_rect, SDL_Color clr, Histogram *h)
{
	const int padding = 10;

	SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);

	/* Draw gray rectangle as background */
	SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 96);
	SDL_RenderFillRect(ctx->renderer, &bounds_rect);

	/* Draw inner rect */
	SDL_Rect inner_rect = {bounds_rect.x + padding, bounds_rect.y + padding, bounds_rect.w - 2 * padding, bounds_rect.h - 2 * padding};
	SDL_SetRenderDrawColor(ctx->renderer, clr.r, clr.g, clr.b, 32);
	SDL_RenderFillRect(ctx->renderer, &inner_rect);

	/* Most likely there won't be enough space to visualize all 256 values, so interpolate them to 128 */
	SDL_SetRenderDrawColor(ctx->renderer, clr.r, clr.g, clr.b, 128);
	int bar_width = inner_rect.w / 128;

	int i;
	for(i=0; i<128; i++) {
		int v = (h->values[i*2] + h->values[i*2+1]) / 2;
		int bar_height = inner_rect.h * v / h->maxval;

		SDL_Rect bar_rect = {inner_rect.x + i * bar_width, inner_rect.y + inner_rect.h - bar_height, bar_width, bar_height};
		SDL_RenderFillRect(ctx->renderer, &bar_rect);
	}

	/* Draw range */
	SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 64);

	SDL_Rect left_marker_rect = {inner_rect.x + (h->min/2) * bar_width, bounds_rect.y, 1, bounds_rect.h};
	SDL_RenderFillRect(ctx->renderer, &left_marker_rect);

	SDL_Rect right_marker_rect = {inner_rect.x + (h->max/2) * bar_width, bounds_rect.y, 1, bounds_rect.h};
	SDL_RenderFillRect(ctx->renderer, &right_marker_rect);

	return RC_OK;
}

RETCODE on_draw(SDLContext *ctx)
{
	int tex_w, tex_h;

	/* Get zoom factor and animate zoom animation (if pending) */
	float zoom_f = ctx->zoom_factor + ctx->zoom_animation_amount * ((float)1-quadratic_easing(ctx->zoom_animation_progress, 0, 1, 1));

	if(ctx->zoom_animation_progress > 0) {
		/* Move animation */
		ctx->zoom_animation_progress -= (float)1 / ZOOM_SPEED;

		/* Animation has ended */
		if(ctx->zoom_animation_progress <= 0) {
			ctx->zoom_animation_progress = 0;
			ctx->zoom_factor += ctx->zoom_animation_amount;
			ctx->zoom_animation_amount = 0;
		}
	}

	/* Get texture's size so we can calculate where to place it on the screen */
	SDL_QueryTexture(ctx->orig_image, NULL, NULL, &tex_w, &tex_h);
	tex_w = (int)((float)tex_w * zoom_f);
	tex_h = (int)((float)tex_h * zoom_f);

	if(ctx->dual_view) {
		int cx = ctx->renderer_size.x / 2;
		int cy = ctx->renderer_size.y / 2 - tex_h / 2;

		/* Center the image on the screen */
		SDL_Rect target_rect1 = {cx - tex_w, cy, tex_w, tex_h};
		SDL_Rect target_rect2 = {cx, cy, tex_w, tex_h};

		/* Draw textures on the backbuffer */
		SDL_RenderCopy(ctx->renderer, ctx->orig_image, NULL, &target_rect1);
		SDL_RenderCopy(ctx->renderer, ctx->filtered_image, NULL, &target_rect2);
	}else {
		/* Center the image on the screen */
		SDL_Rect target_rect = {ctx->renderer_size.x / 2 - tex_w / 2, ctx->renderer_size.y / 2 - tex_h / 2, tex_w, tex_h};

		/* Draw texture on the backbuffer */
		SDL_RenderCopy(ctx->renderer, ctx->filtered_image, NULL, &target_rect);
	}

	/* Draw histograms */
	if(ctx->show_histograms) {
		const int hist_padding = 15;
		const int hist_height = (int)((float)((ctx->renderer_size.x - hist_padding) / 3 - 10) * 0.55555);
		SDL_Rect hist_rect = {hist_padding, hist_padding, ctx->renderer_size.x - hist_padding, hist_height};
		int hist_w = hist_rect.w / 3 - 10;

		/* Draw red histogram */
		SDL_Rect tmp_rect = {hist_rect.x + 5, hist_rect.y, hist_w, hist_rect.h};
		SDL_Color tmp_color = {255, 0, 0, 1};
		draw_histogram(ctx, tmp_rect, tmp_color, &ctx->histograms[0]);

		/* Draw green histogram */
		SDL_Rect tmp_rect2 = {hist_rect.x + 10 + hist_w, hist_rect.y, hist_w, hist_rect.h};
		SDL_Color tmp_color2 = {0, 255, 0, 1};
		draw_histogram(ctx, tmp_rect2, tmp_color2, &ctx->histograms[1]);

		/* Draw blue histogram */
		SDL_Rect tmp_rect3 = {hist_rect.x + 15 + hist_w * 2, hist_rect.y, hist_w, hist_rect.h};
		SDL_Color tmp_color3 = {0, 0, 255, 1};
		draw_histogram(ctx, tmp_rect3, tmp_color3, &ctx->histograms[2]);
	}

	return RC_OK;
}

RETCODE on_key_down(SDLContext *ctx, SDL_Keycode kc)
{
#define zoom_step	0.15
	switch(kc) {
	case SDLK_KP_PLUS:
		//ctx->zoom_factor += zoom_step;
		if(ctx->zoom_animation_progress == 0) {
			ctx->zoom_animation_progress += 1;
			ctx->zoom_animation_amount += zoom_step;
		}

		break;

	case SDLK_KP_MINUS:
		if(ctx->zoom_factor >= zoom_step) {
			//ctx->zoom_factor -= zoom_step;
			if(ctx->zoom_animation_progress == 0) {
				ctx->zoom_animation_progress = 1;
				ctx->zoom_animation_amount -= zoom_step;
			}
		}
		break;

	case SDLK_0:
		printf("Reseting to original image.\n");
		copy_texture(ctx->orig_image, ctx->filtered_image);
		histogram_extract(ctx->filtered_image, &ctx->histograms[0], &ctx->histograms[1], &ctx->histograms[2]);
		break;

	case SDLK_h:
		/* Toggle histograms' visibility */
		ctx->show_histograms = !ctx->show_histograms;
		break;

	case SDLK_s:
		/* Save filtered image to PGM file */
		printf("Saving image to file \"%s\"...\n", "output.pgm");
		image_save_to_file("output.pgm", "pgm", ctx->filtered_image);
		break;

	case SDLK_d:
		ctx->dual_view = !ctx->dual_view;
		break;
	}

	/* Handle keys [1..9] for applying filters */
	if(kc >= SDLK_1 && kc <= SDLK_9) {
		Filter2D *f;

		if(filter_find_by_id(kc - SDLK_1, &f) == RC_OK) {
			printf("Applying image filter \"%s\".\n", f->name);
			filter_apply_to_texture(ctx->orig_image, ctx->filtered_image, f->name);
			histogram_extract(ctx->filtered_image, &ctx->histograms[0], &ctx->histograms[1], &ctx->histograms[2]);
		}
	}

	return RC_OK;
}

RETCODE mainloop(SDLContext *ctx)
{
	int quit = 0;
	SDL_Event event;
	RETCODE rc;

	/* While application is running */
	while(!quit) {
		/* Handle events in the queue */
		while(SDL_PollEvent(&event) != 0) {
			switch(event.type) {
			case SDL_QUIT:
				quit = 1;
				break;

			case SDL_KEYDOWN:
				if(event.key.keysym.scancode == SDL_SCANCODE_Q)
					quit = 1;
				else
					on_key_down(ctx, event.key.keysym.sym);
				break;
			}
		}

        /* Use black for background color */
		if (SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255) != 0) {
			return RC_FAIL;
		}

		/* Clear back buffer */
		if (SDL_RenderClear(ctx->renderer) != 0) {
			return RC_FAIL;
		}

		/* Draw the screen */
		rc = on_draw(ctx);
		if(failed(rc)) {
			printf("Fatal error occured while drawing (rc=%d)...\n", rc);
			return rc;
		}

		/* Present back buffer */
		SDL_RenderPresent(ctx->renderer);
	}

	return RC_OK;
}

int main(int argc, char** argv){
    int i;

	SDLContext *ctx = NULL;
	//char filename[1024] = "images/lena.ascii.pgm";
	RETCODE rc;

	/* Check if first parameter is present */
	if(argc<2) {
		char *fn = strrchr(argv[0], '\\') + 1;
		printf("No image file selected.\nUsage: \"%s <image.pgm>\"\n", fn);
		return 0;
	}

	SDL_Rect wnd_rect = {SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 890, 680};
	SDL_Point texture_size = {512, 512};

	/* Initialize SDL framework */
	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		printf("SDL_Init Error: %s\n", SDL_GetError());
		return 1;
	}

	printf("Creating SDL window and renderer...");
	rc = sdl_ctx_init(&ctx, wnd_rect, texture_size);
	if(failed(rc)) {
		printf("failed\n");
		goto uninit_sdl;
	}else {
		printf("done\n");
	}

	printf("Loading image file \"%s\"...", argv[1]);
	rc = sdl_ctx_alloc_textures(ctx, argv[1]);
	if(failed(rc)) {
		printf("failed\n");
		goto uninit_sdl;
	}else {
		printf("done\n");
	}

	copy_texture(ctx->orig_image, ctx->filtered_image);
	histogram_extract(ctx->filtered_image, &ctx->histograms[0], &ctx->histograms[1], &ctx->histograms[2]);

	/* Print navigation info */
	printf("\nUse the following keys for the respective operation...\n");
	printf("[+/-] Zoom in/out (from the num pad)\n");
	printf("[1..9] Apply filters\n");
	printf("[0] Reset to original image\n");
	printf("[H] Toggle histograms\n");
	printf("[D] Toggle dual image view\n");
	printf("[S] Save filtered image\n");
	printf("[Q] Quit\n");
	printf("\nPress any key to continue...\n");
	//getch();

	/* Enter the app loop */
	printf("Entering main loop (press \"Q\" to exit the program)...\n");
	mainloop(ctx);

uninit_sdl:
	printf("Exiting...\n");
	sdl_ctx_dispose(&ctx);
	SDL_Quit();
	return 0;
}
