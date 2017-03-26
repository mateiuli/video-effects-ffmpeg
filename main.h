#ifndef MAIN_H
#define MAIN_H

#include "ctve.h"
#include <math.h>

typedef struct {
	char inFile[128];
	char outFile[128];
	char effect[32];
	float value[3];
} conf_t;

/* Grab user's configuration. */
int parse_args(char **argv, int argc, conf_t *conf);

/* Process some frames. */
void process_blur(ctve_video_t *video);
void process_bw(ctve_video_t *video);
void process_sepia(ctve_video_t *video);
void process_saturation(ctve_video_t *video);

#endif