#include <stdio.h>
#include <stdint.h>
#include "packet.h"

Command *commands; // Create a list of function pointers. Take list of 8 bit characters
uint8_t commandsLength;
Board recAddr;

/**
 * Populate the commands in the file. The index of the command processed in the packet is equivalent to the position
 * in the array. If the command is in the 0 position, it corresponds to a 0 in the command.
 * Errors:
 * -1 corresponds to the desired command list having no length.
 * @param des_commandLength The length of the command list
 * @param des_commands The list of desired commands in the form of a Command typedef.
 * @return Returns status.
 */
uint8_t init(Board des_recAddr, uint8_t des_commandLength, Command des_commands[]) {
    if(des_commands == NULL) {
        return -1;
    }
    commandsLength = des_commandLength;
    commands = des_commands;
    recAddr = des_recAddr;
    return 0;
}


/**
 * Parse a packet, make sure it was intended for this board, and call the requested command method with the parameters from the packet.

 * Return Codes: 
 *  1: Packet destination not this board
 * -1: Packet length of 0
 * -2: Invalid Command Recieved
 * @param packet Array of packet bytes
 * @param length Length of packet
 * @return Returns status.
 */
int8_t parsePacket(uint8_t *packet, size_t length) {
    if(length == 0) {
        return -1; 
    }

    Board address = (Board)(packet[0] >> 6);
    if (address != recAddr) {
        return 1;
    }
    uint8_t command = packet[0] & 0x3f;

    if(command >= commandsLength) {
        return -2;
    }
    commands[command](packet + 1);
    return 0;
}