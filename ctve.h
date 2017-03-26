/**
* CTVE stands for Command line Tool for Video Enhancement
* It features options for video resizing, blur and color correction.
* It works only with MPEG videos, not AVI, MP4, MKV, FLV, etc.
*/

#ifndef CTVE_H
#define CTVE_H

#include <math.h>
#include <ctype.h>

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>

#define INBUF_SIZE 		4096
#define FRAMES_COUNT	30

/**
 * How many bytes does a pixel have.
 */
typedef enum 
{
	BW = 1,
	RGB = 3,
} ctve_frame_pixel_t;

/**
 * Struct that represents only a single frame.
 */
typedef struct 
{
	/* Frame bytes. */
	uint8_t *data;
	/* Bytes count.*/
	uint32_t	 length;

	/* Frame size - in pixels.*/
	uint16_t width;
	uint16_t height;

	/* Pixel size. RGB = 3. B/W = 1. */
	ctve_frame_pixel_t pixel_type;
} ctve_frame_t;

void SaveFrame2(uint8_t *data, int width, int height, int iFrame);

/**
 * Struct that represents an entire video.
 * A video has multiple frames.
 */
typedef struct
{
	/* All frames, in order. */
	ctve_frame_t *frames;
	/* How many frames are in there. */
	uint32_t length;

	/* Overall video size.*/
	uint16_t width;
	uint16_t height;

	/* Frame-rate - how many frames does a second have. */
	float frame_rate;
} ctve_video_t;

/** 
 * Pointer to function which gets called to process X frames before
 * writing them into an output file. 
 */
typedef void (*cvte_algorithm_func)(ctve_video_t*);

/**
 * Creates an empty frame with a given size.
 * The frame should be free'd with ctve_free_frame().
 */
ctve_frame_t *ctve_create_frame_empty(uint16_t width, uint16_t height, ctve_frame_pixel_t pixel_type);

/**
 * Creates a frame from a given buffer. It copies the buffer (deep copy).
 * The frame should be free'd with ctve_free_frame().
 */
ctve_frame_t *ctve_create_frame(uint8_t *data, uint16_t width, uint16_t height, ctve_frame_pixel_t pixel_type);

/**
 * Frees up the entire frame. 
 * All pointers are set to NULL.
 */
void ctve_free_frame(ctve_frame_t *frame);

/**
 * Creates an empty video with a given size.
 * The video should be free'd with ctve_free_video().
 */
ctve_video_t *ctve_create_video_empty(uint16_t width, uint16_t height, float frame_rate);

/**
 * Creates a video from an array of frames.
 * IT DOES NOT COPY THE FRAMES! Make sure you don't delete 'em!
 * The size of the video will equal to the size of the first frame.
 * The video should be free'd with ctve_free_video().
 */
ctve_video_t *ctve_create_video(ctve_frame_t* frames, uint32_t len, float frame_rate);

/**
 * Frees up the entire video. First deletes all the frames!
 * All pointers are set to NULL.
 */
void ctve_free_video(ctve_video_t *video);

/**
 * Loads a video from file and returns a ctve_vide_t.
 */
ctve_video_t *ctve_load_and_process_video(const char *infile, const char *outfile, cvte_algorithm_func func);

#endif