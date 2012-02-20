#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#define INIT_GRAPHICS 0
#define GET_FRAME_BUFFER_INFO	1

struct frame_buffer_info {
	int width;
	int height;
	int bpp;
	void *base_address;
	int bytes_per_row;
};

#endif
