#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string.h>
#include <stdint.h>

#define RAW_MAX_VALUE 4000
#define RAW_MIN_VALUE 0

#define WHEEL_MAX_VALUE 150
#define WHEEL_MIN_VALUE -150

#define GAS_MIN_VALUE 0
#define GAS_MAX_VALUE 120

#define BRAKE_MIN_VALUE 0
#define BRAKE_MAX_VALUE 120

typedef struct {
  int fd;
  char name[50];
} simulation_controller_t;

typedef struct
{
  uint32_t raw_axis;
  uint32_t raw_gas;
  uint32_t raw_brake;
} raw_input_t;

typedef struct
{
  int16_t axis;
  uint8_t gas;
  uint8_t brake;
} controller_input_t;


simulation_controller_t* create_controller(const char *name);

int emit(simulation_controller_t *controller, int type, int code, int val, int emit_syn);

int emit_btn(simulation_controller_t *controller, int btn, uint8_t val);

int emit_wheel(simulation_controller_t *controller, int16_t val);

int emit_brake(simulation_controller_t *controller, uint8_t val);

int emit_gas(simulation_controller_t *controller, uint8_t val);

int remove_controller(simulation_controller_t *controller);

int setup_controller(simulation_controller_t *controller);

#endif
