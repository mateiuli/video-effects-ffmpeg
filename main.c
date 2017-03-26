#include <stdio.h>
#include <string.h>

#include "main.h"
#include "blur.h"
#include "effects.h"

#include <sys/time.h>

/* Global configuration. */
static conf_t conf;

int main(int argc, char **argv)
{
	ctve_video_t *video;
	cvte_algorithm_func effect = NULL;

	if(argc < 4) {
		printf("Usage: %s <input_file> <output_file> <effect_name> [<arg1> [<arg2> [<arg3..]]\n\n", argv[0]);
		printf("[Available effects]\n");
		printf("\t1) bw\n");
		printf("\t2) sepia\n");
		printf("\t3) blur [<value>] - default values is 5\n");
		printf("\t3) saturation <red> <green> <blue> - In range [0..2]\n");
		printf("\n");
		return -1;
	}

	/* Parse arguments. */
	parse_args(argv, argc, &conf);

	/* Init blur radius. Default is 5 even if other effect is requested. */
	blur_init((int)conf.value[0]);

	if(strcmp(conf.effect, "bw") == 0) {
		/* Apply black and white filter. */
		effect = process_bw;
		printf("Effect: black and white\n");
	} else if(strcmp(conf.effect, "sepia") == 0) {
		/* Apply sepia filter. */
		effect = process_sepia;
		printf("Effect: sepia\n");
	} else if(strcmp(conf.effect, "saturation") == 0) {
		/* Apply sepia filter. */
		effect = process_saturation;
		printf("Effect: saturation, r = %f, g = %f, b = %f\n", conf.value[0], conf.value[1], conf.value[2]);
	}else if(strcmp(conf.effect, "blur") == 0) {
		/* Apply blur effect. */
		effect = process_blur;
		printf("Effect: blur, %f\n", conf.value[0]);
	}

	if(effect == NULL) {
		printf("Requested effect is not implemented.\n");
		return -1;
	}
	
	struct timeval begin, end;
	gettimeofday(&begin, NULL);

	/* Apply effect and write outfile. */
	video = ctve_load_and_process_video(conf.inFile, conf.outFile, effect);

	
	gettimeofday(&end, NULL);
	double elapsed = (end.tv_sec - begin.tv_sec) + 
              ((end.tv_usec - begin.tv_usec)/1000000.0);

	printf("Time: %lf\n", elapsed);

	ctve_free_video(video);

	/* Free resources. */
	blur_free();

	return 0;
}

int parse_args(char **argv, int argc, conf_t *conf)
{
	conf->value[0] = 5.f;

	memcpy(conf->inFile, argv[1], strlen(argv[1]) + 1);
	memcpy(conf->outFile, argv[2], strlen(argv[2]) + 1);
	memcpy(conf->effect, argv[3], strlen(argv[3]) + 1);

	if(strcmp(conf->effect, "blur") == 0 && argc == 5)
		sscanf(argv[4], "%f", &conf->value[0]);
	else if(strcmp(conf->effect, "saturation") == 0 && argc == 7) {
		sscanf(argv[4], "%f", &conf->value[0]);
		sscanf(argv[5], "%f", &conf->value[1]);
		sscanf(argv[6], "%f", &conf->value[2]);
	}
}

/**
 * This function gets called after every FRAMES_COUNT frames.
 * They get read and saved into a video structure in RGB format.
 * This function role is to alter the frames - do the processing.
 * The saving is done automatically.
 */
void process_blur(ctve_video_t *video)
{
	static int count = 0;
	for(int i = 0; i < video->length; ++i) {
		blur_apply(&video->frames[i]);

		if(count < 5)
			SaveFrame2(video->frames[i].data, video->frames[i].width, video->frames[i].height, count++);
	}
}

void process_bw(ctve_video_t *video)
{
	static int count = 0;
	for(int i = 0; i < video->length; ++i)
		effects_apply_bw(&video->frames[i]);
}

void process_sepia(ctve_video_t *video)
{
	for(int i = 0; i < video->length; ++i)
		effects_apply_sepia(&video->frames[i]);
}

void process_saturation(ctve_video_t *video)
{
	for(int i = 0; i < video->length; ++i)
		effects_saturation(&video->frames[i], conf.value[0], conf.value[1], conf.value[2]);
}