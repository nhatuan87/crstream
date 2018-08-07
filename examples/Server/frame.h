#ifndef FRAME_H
#define FRAME_H

typedef struct {
	float    analogA0;
	uint8_t	 ledStatus;
} req_frame;

typedef struct {
	uint32_t networkTime;
	uint8_t  ledStatus;
} res_frame;

#endif
