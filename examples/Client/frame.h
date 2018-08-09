#ifndef FRAME_H
#define FRAME_H

typedef struct {
	float    analogA0;
	bool	 ledStatus;
} req_frame;

typedef struct {
	uint32_t networkTime;
	bool	 ledStatus;
} res_frame;

#endif