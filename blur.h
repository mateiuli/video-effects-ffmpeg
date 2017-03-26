#ifndef BLUR_H
#define BLUR_H

#include "ctve.h"

/* Initialize internal kernel. */
void blur_init(int radius);

/* Release internal memory used by the kernel. */
void blur_free();

/* Apply blur effect on a RGB frame. */
void blur_apply(ctve_frame_t *frame);

#endif
