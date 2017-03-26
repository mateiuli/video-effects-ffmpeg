#include "effects.h"
#include "util.h"

void effects_apply_bw(ctve_frame_t *frame)
{
	if(!frame)
		return;

	for(int i = 0; i < frame->height; ++i) {
		for(int j = 0; j < 3 * frame->width; j += 3) {
			int grayscale = (frame->data[i * 3 * frame->width + j]
							+ frame->data[i * 3 * frame->width + j + 1]
							+ frame->data[i * 3 * frame->width + j + 2]) / 3;


			frame->data[i * 3 * frame->width + j] = grayscale;
			frame->data[i * 3 * frame->width + j + 1] = grayscale;
			frame->data[i * 3 * frame->width + j + 2] = grayscale;
		}
	}
}

void effects_apply_sepia(ctve_frame_t *frame)
{
	if(!frame)
		return;

	for(int i = 0; i < frame->height; ++i) {
		for(int j = 0; j < 3 * frame->width; j += 3) {
			int r = frame->data[i * 3 * frame->width + j];
			int g = frame->data[i * 3 * frame->width + j + 1];
			int b = frame->data[i * 3 * frame->width + j + 2];

			frame->data[i * 3 * frame->width + j] 		= MIN(r * 0.393f + g * 0.769f + b * 0.189, 255);
			frame->data[i * 3 * frame->width + j + 1] 	= MIN(r * 0.349f + g * 0.686f + b * 0.168, 255);
			frame->data[i * 3 * frame->width + j + 2] 	= MIN(r * 0.272f + g * 0.534f + b * 0.131, 255);
		}
	}
}

void effects_saturation(ctve_frame_t *frame, float kR, float kG, float kB)
{
	if(!frame)
		return;

	for(int i = 0; i < frame->height; ++i) {
		for(int j = 0; j < 3 * frame->width; j += 3) {
			int r = frame->data[i * 3 * frame->width + j];
			int g = frame->data[i * 3 * frame->width + j + 1];
			int b = frame->data[i * 3 * frame->width + j + 2];

			frame->data[i * 3 * frame->width + j] 		= MIN(r * kR, 255);
			frame->data[i * 3 * frame->width + j + 1] 	= MIN(g * kG, 255);
			frame->data[i * 3 * frame->width + j + 2] 	= MIN(b * kB, 255);
		}
	}
}
