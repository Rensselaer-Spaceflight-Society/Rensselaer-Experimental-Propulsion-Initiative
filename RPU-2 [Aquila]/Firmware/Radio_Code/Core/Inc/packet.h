#ifndef __PACKET_H
#define __PACKET_H

typedef void (*Command)(uint8_t*);

typedef enum {
	CONTROL_BOARD = 0,
	FILL_CONTROL = 1,
	DAQ_CONTROL = 2,
	RESERVED = 3
} Board;

uint8_t init(Board des_recAddr, uint8_t des_commandLength, Command des_commands[]);
int8_t parsePacket(uint8_t *packet, size_t length);

#endif