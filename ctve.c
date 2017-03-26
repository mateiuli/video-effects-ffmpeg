#include "ctve.h"

static cvte_algorithm_func algorithm_func = NULL;

/* Video output. */
static FILE *outFile;
static AVFrame *outFrame;
static AVPacket outPkt;
static AVCodec *outCodec;
static AVCodecContext *outContext;
static struct SwsContext *outSwsContext;
static uint8_t outEndcode[] = { 0, 0, 1, 0xb7 };
static int outWrites        = 0;

ctve_frame_t *ctve_create_frame_empty(uint16_t width, uint16_t height, ctve_frame_pixel_t pixel_type)
{
    ctve_frame_t *frame = (ctve_frame_t*)malloc(sizeof(ctve_frame_t));

    /* Initialize frame. */
    frame->width = width;
    frame->height = height;
    frame->pixel_type = pixel_type;
    frame->length = width * height * (int)pixel_type;

    /* Alloc buffer. */
    frame->data = (uint8_t*)malloc(frame->length * sizeof(uint8_t));

    return frame;
}

ctve_frame_t *ctve_create_frame(uint8_t *data, uint16_t width, uint16_t height, ctve_frame_pixel_t pixel_type)
{
    /* Creates an empty frame. */
    ctve_frame_t *frame = ctve_create_frame_empty(width, height, pixel_type);

    /* Copy the data into the frame. */
    int bytes = width * height * (int)pixel_type;
    memcpy(frame->data, data, bytes);

    return frame;
}

void ctve_init_frame(ctve_frame_t *frame, uint8_t *data, uint16_t width, uint16_t height, ctve_frame_pixel_t pixel_type)
{
    /* Initialize frame. */
    frame->width = width;
    frame->height = height;
    frame->pixel_type = pixel_type;
    frame->length = width * height * (int)pixel_type;

    /* Alloc buffer. */
    frame->data = (uint8_t*)malloc(frame->length * sizeof(uint8_t));

    /* Copy the data into the frame. */
    memcpy(frame->data, data, frame->length);
}

void ctve_free_frame(ctve_frame_t *frame)
{
    if(frame == NULL)
        return;

    if(frame->data != NULL)
        free(frame->data);

    //free(frame);
}

ctve_video_t *ctve_create_video_empty(uint16_t width, uint16_t height, float frame_rate)
{
    /* Alloc an empty video struct. */
    ctve_video_t *video = (ctve_video_t*)malloc(sizeof(ctve_video_t));

    /* Init fields. */
    video->length       = 0;
    video->frames       = NULL;
    video->width        = width;
    video->height       = height;
    video->frame_rate   = frame_rate;

    return video;
}

ctve_video_t *ctve_create_video(ctve_frame_t* frames, uint32_t len, float frame_rate)
{
    if(frames == NULL || len <= 0)
        return NULL;

    uint16_t width  = frames[0].width;
    uint16_t height = frames[0].height;

    /* Create an empty video structure. */
    ctve_video_t *video = ctve_create_video_empty(width, height, frame_rate);

    /* Attach frames. */
    video->length = len;
    video->frames = frames;

    return video;
}

void ctve_free_video(ctve_video_t *video)
{
    if(video == NULL)
        return;

    /* Firstly, free up frames.*/
    for(int i = 0; i < video->length; ++i)
        ctve_free_frame(&video->frames[i]);

    free(video);
}

static void ctve_open_out_file(const char *outfile, AVCodecContext *codecCtx, ctve_video_t *video, int codec_id)
{
    int ret;
    printf("Encode video file %s\n", outfile);

    /* find the video encoder */
    outCodec = avcodec_find_encoder(codec_id);
    if (!outCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    outContext = avcodec_alloc_context3(outCodec);
    if (!outContext) {
        fprintf(stderr, "Could not allocate video outCodec context\n");
        exit(1);
    }

    /* put sample parameters */
    outContext->bit_rate = codecCtx->bit_rate;
    /* resolution must be a multiple of two */
    outContext->width = video->width;
    outContext->height = video->height;
    /* frames per second */
    outContext->time_base = (AVRational){1, video->frame_rate};
    outContext->gop_size = codecCtx->gop_size;
    outContext->max_b_frames = codecCtx->max_b_frames;
    outContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(outContext->priv_data, "preset", "slow", 0);

    /* open it */
    if (avcodec_open2(outContext, outCodec, NULL) < 0) {
        fprintf(stderr, "Could not open outCodec\n");
        exit(1);
    }

    outFile = fopen(outfile, "wb");
    if (!outFile) {
        fprintf(stderr, "Could not open %s\n", outfile);
        exit(1);
    }

    outFrame = av_frame_alloc();
    if (!outFrame) {
        fprintf(stderr, "Could not allocate video outFrame\n");
        exit(1);
    }

    outFrame->format = outContext->pix_fmt;
    outFrame->width  = outContext->width;
    outFrame->height = outContext->height;

    outSwsContext = sws_getContext(
        outFrame->width, 
        outFrame->height,
        AV_PIX_FMT_RGB24, 
        outFrame->width, 
        outFrame->height,
        AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(
        outFrame->data, 
        outFrame->linesize, 
        outContext->width, 
        outContext->height,
        outContext->pix_fmt, 
        32
    );

    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }
}

static void ctve_write_out_file(ctve_video_t *video)
{
    int got_output, i;
    static int last = 0;
    int inLineSize[1] = {3 * video->width};
    
    /* encode 1 second of video */
    for (i = 0; i < video->length; i++) {
        av_init_packet(&outPkt);
        outPkt.data = NULL;    // packet data will be allocated by the encoder
        outPkt.size = 0;
        
        uint8_t *inData[1] = {video->frames[i].data};

        sws_scale(
            outSwsContext, 
            (const uint8_t * const *)inData,
            inLineSize, 
            0,
            video->height, 
            outFrame->data, 
            outFrame->linesize
        );

        outFrame->pts = last++;

        /* encode the image */
        int ret = avcodec_encode_video2(outContext, &outPkt, outFrame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding outFrame1\n");
            exit(1);
        }

        if (got_output) {
            fwrite(outPkt.data, 1, outPkt.size, outFile);
            av_packet_unref(&outPkt);
        }
    }
}

static void ctve_save_frame(ctve_video_t *video, uint8_t *data, int linesize)
{
    if(video == NULL || data == NULL)
        return;

    if(video->frames == NULL) {
        /* First chunk of frames. */
        video->frames = (ctve_frame_t*)malloc(FRAMES_COUNT * sizeof(ctve_frame_t));

        for(int i = 0; i < FRAMES_COUNT; ++i) {
            /* Init frame. */
            video->frames[i].width = video->width;
            video->frames[i].height = video->height;
            video->frames[i].pixel_type = RGB;
            video->frames[i].length = video->width * video->height * (int)RGB;
            video->frames[i].data = (uint8_t*)malloc(video->frames[i].length);
        }
    }

    if(video->length == FRAMES_COUNT) {
        /* Process these frames. */
        if(algorithm_func != NULL)
            algorithm_func(video);

        /* Write these frames into output file.*/
        ctve_write_out_file(video);

        /* Reset length for the next 30 frames. */
        video->length = 0;
    }

    /* Access current frame. */
    ctve_frame_t *frame = &video->frames[video->length];

    /* Copy data from AVFrame into frame's local data. */
    uint8_t *p = frame->data;

    int len = frame->width * (int)frame->pixel_type;
    for(int i = 0; i < frame->height; ++i) {
        memcpy(p, data + i * linesize, len);
        p += len;
    }

    /* Increment the number of frames.*/
    video->length++;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y * pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);
}


void SaveFrame2(uint8_t *data, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(data + y * 3 * width, 1, width * 3, pFile);
  
  // Close file
  fclose(pFile);
}

ctve_video_t *ctve_load_and_process_video(const char *infile, const char *outfile, cvte_algorithm_func func)
{
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL; 
    AVFrame         *pFrameRGB = NULL;
    AVPacket        packet;
    int             frameFinished;
    int             numBytes;
    uint8_t         *buffer = NULL;

    AVDictionary    *optionsDict = NULL;
    struct SwsContext      *sws_ctx = NULL;

    // Init process function
    algorithm_func = func;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(avformat_open_input(&pFormatCtx, infile, NULL, NULL) != 0)
        return NULL; // Couldn't open file

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return NULL; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, infile, 0);

    // Find the first video stream
    videoStream = -1;
    for(i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if(videoStream == -1)
        return NULL; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return NULL; // Codec not found
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0)
        return NULL; // Could not open codec

    // Allocate video frame
    pFrame = av_frame_alloc();

    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if(pFrameRGB == NULL)
        return NULL;

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    /* Our very own video container. The width/height get initialized when first frame is copied.*/
    ctve_video_t *video = ctve_create_video_empty(pCodecCtx->width, pCodecCtx->height, 30);

    sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        PIX_FMT_RGB24,
        SWS_BICUBIC,
        NULL,
        NULL,
        NULL
    );

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
         pCodecCtx->width, pCodecCtx->height);

    // Read frames and save first five frames to disk
    i = 0;
    while(av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
          
            // Did we get a video frame? 
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale(
                    sws_ctx,
                    (uint8_t const * const *)pFrame->data,
                    pFrame->linesize,
                    0,
                    pCodecCtx->height,
                    pFrameRGB->data,
                    pFrameRGB->linesize
                );

                if(i == 0)
                    ctve_open_out_file(outfile, pCodecCtx, video, pCodecCtx->codec_id);

                /* Copy frame into video structure.*/
                ctve_save_frame(video, pFrameRGB->data[0], pFrameRGB->linesize[0]);

                i++;
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // Out file.
    /* get the delayed frames */
    for (int got_output = 1; got_output; i++) {
        fflush(stdout);

        int ret = avcodec_encode_video2(outContext, &outPkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding outFrame2\n");
            exit(1);
        }

        if (got_output) {
            fwrite(outPkt.data, 1, outPkt.size, outFile);
            av_packet_unref(&outPkt);
        }
    }

    /* add sequence end code to have a real MPEG file */
    fwrite(outEndcode, 1, sizeof(outEndcode), outFile);
    fclose(outFile);

    avcodec_close(outContext);
    av_free(outContext);
    av_freep(&outFrame->data[0]);
    av_frame_free(&outFrame);

    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);

    // Free the YUV frame
    av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);
    
    return video;
}