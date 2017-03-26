#ifndef EFFECTS_H
#define EFFECTS_H

#include "ctve.h"

/* Convert frame into black and white. */
void effects_apply_bw(ctve_frame_t *frame);

/* Sepia filter. */
void effects_apply_sepia(ctve_frame_t *frame);

/* Color saturation. */
void effects_saturation(ctve_frame_t *frame, float r, float g, float b);

#endif
