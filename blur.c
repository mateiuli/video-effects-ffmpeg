#include "blur.h"
#include "util.h"

static int bRadius;
static int bRadiusSq;
static int *bDirRow;
static int *bDirCol;

void blur_init(int radius)
{
	int index = 0;
	
	if(radius < 0)
		return;
	else if(radius % 2 == 0)
		radius++;
	
	bRadius = radius;
	bRadiusSq = radius * radius;

	bDirRow = (int*)malloc((bRadiusSq + 1) * sizeof(int));
	bDirCol = (int*)malloc((bRadiusSq + 1) * sizeof(int));

	for(int i = -radius / 2; i <= radius / 2; ++i) 
		for(int j = -radius / 2; j <= radius / 2; ++j) {
			bDirRow[index] 	= i;
			bDirCol[index] = j;
			++index;
		}
}

void blur_free()
{
	free(bDirRow);
	free(bDirCol);
}

void blur_apply(ctve_frame_t *frame)
{
	if(!frame || !bDirCol || !bDirCol)
		return;

	for(int i = 0; i < frame->height; ++i) {
		for(int j = 0; j < 3 * frame->width; j += 3) {
			float r = 0, g = 0, b = 0;

			for(int dir = 0; dir < bRadiusSq; ++dir) {
				int newY = MAX(0, MIN(i + bDirRow[dir], frame->height - 1));
				int newX = MAX(0, MIN(j + 3 * bDirCol[dir], 3 * frame->width - 1));

				r += (float)frame->data[newY * 3 * frame->width + newX] / (float)bRadiusSq;
				g += (float)frame->data[newY * 3 * frame->width + newX + 1] / (float)bRadiusSq;
				b += (float)frame->data[newY * 3 * frame->width + newX + 2] / (float)bRadiusSq;
			}

			frame->data[i * 3 * frame->width + j] = r;
			frame->data[i * 3 * frame->width + j + 1] = g;
			frame->data[i * 3 * frame->width + j + 2] = b;
		}
	}
}
