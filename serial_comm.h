#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>

int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);


#endif