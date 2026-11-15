#ifndef __PACKET_H
#define __PACKET_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef void (*Command)(uint8_t*);

typedef enum {
	CONTROL_BOARD = 0,
	FILL_CONTROL = 1,
	DAQ_CONTROL = 2,
	RESERVED = 3
} Board;

uint8_t packetInit(Board des_recAddr, uint8_t des_commandLength, Command des_commands[]);
int8_t parsePacket(Command *desiredCommand, char *packet, size_t length);
uint8_t createPacket(uint8_t *packet, Board dest, uint8_t command, uint8_t *payload, uint8_t payload_size);

#endif
