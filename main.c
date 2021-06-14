#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include "controller.h"
#include "serial_comm.h"

simulation_controller_t *controller;
raw_input_t raw_input;
controller_input_t actual_input;

int main(int argc, char const *argv[])
{
    int rc;
    controller = create_controller("Simulation controller");
    if(setup_controller(controller) < 0)
        printf("Controller setup error!\n");
    
    char *portname = "/dev/ttyACM0";
    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error opening %s", portname);
        return 0;
    }
    set_interface_attribs (fd, B9600, 0);  
    set_blocking (fd, 0);
    
    while(1)
    {
        rc = read (fd, &raw_input, sizeof(raw_input_t));
        if(rc == sizeof(raw_input_t))
        {
            printf("RAW | Axis: %d, Gas: %d, Brake: %d\n", raw_input.raw_axis, raw_input.raw_gas, raw_input.raw_brake);
            actual_input.axis =  (raw_input.raw_axis - RAW_MIN_VALUE)*(WHEEL_MAX_VALUE-WHEEL_MIN_VALUE)/(RAW_MAX_VALUE-RAW_MIN_VALUE) + WHEEL_MIN_VALUE;
            actual_input.gas =  (raw_input.raw_gas - RAW_MIN_VALUE)*(GAS_MAX_VALUE-GAS_MIN_VALUE)/(RAW_MAX_VALUE-RAW_MIN_VALUE) + GAS_MIN_VALUE;
            actual_input.brake =  (raw_input.raw_brake - RAW_MIN_VALUE)*(BRAKE_MAX_VALUE-BRAKE_MIN_VALUE)/(RAW_MAX_VALUE-RAW_MIN_VALUE) + BRAKE_MIN_VALUE;
            printf("ACTUAL | Axis: %d, Gas: %d, Brake: %d\n", actual_input.axis, actual_input.gas, actual_input.brake);
            emit_wheel(controller, actual_input.axis);
            emit_gas(controller, actual_input.gas);
            emit_brake(controller, actual_input.brake);
        }
    }

    return 0;
}
