#include "controller.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdint.h>
#include "macrologger.h"

simulation_controller_t* create_controller(const char *name) {
  simulation_controller_t *controller = (simulation_controller_t*)calloc(1, sizeof(simulation_controller_t));
  strcpy(controller->name, name);
  return controller;
}

int close_controller(simulation_controller_t *controller) {
  LOG_DEBUG("Closing wheel %s", controller->name);
  return close(controller->fd);
}

int add_input(simulation_controller_t *controller, const char *setbit_name, int ev_code,
              int set_bit, int code) {
  int i, res;
  if (ioctl(controller->fd, UI_SET_EVBIT, ev_code) < 0) {
    close_controller(controller);
    return -1;
  }

  if (ioctl(controller->fd, set_bit, code) < 0) {
    close_controller(controller);
    return -1;
  }
  return 0;
}

// int add_wheel_btns(simulation_controller *controller) {
//   LOG_DEBUG("Adding buttons to the wheel");
//   int i;
//   for (i = BTN_TRIGGER_HAPPY; i <= BTN_TRIGGER_HAPPY40; i++) {
//     if (add_input(controller, "UI_SET_KEYBIT", EV_KEY, UI_SET_KEYBIT, i) < 0) {
//       close_wheel(controller);
//       return -1;
//     }
//   }
//   LOG_DEBUG("Done adding buttons to the wheel");
//   return 0;
// }

int add_wheel_abs(simulation_controller_t *controller, int code) {
  if (add_input(controller, "UI_SET_KEYBIT", EV_ABS, UI_SET_ABSBIT, code) < 0) {
    close_controller(controller);
    return -1;
  }
  return 0;
}

int add_wheel_w_pedals(simulation_controller_t *controller) {
  LOG_DEBUG("Actually making the wheel and adding pedals");
  if (add_wheel_abs(controller, ABS_WHEEL) < 0)
    return -1;
  if (add_wheel_abs(controller, ABS_GAS) < 0)
    return -1;
  if (add_wheel_abs(controller, ABS_BRAKE) < 0)
    return -1;
  LOG_DEBUG("Done making the wheel and its pedals.");
  return 0;
}

int emit(simulation_controller_t *controller, int type, int code, int val, int emit_syn) {
  if (code >= BTN_TRIGGER_HAPPY1 && code <= BTN_TRIGGER_HAPPY40) {
    LOG_DEBUG("emit: Emitting button event. value: %d", val);
  } 
  struct input_event ie;

  memset(&ie, 0, sizeof(struct input_event));
  ie.type = type;
  ie.code = code;
  ie.value = val;
  gettimeofday(&ie.time, NULL);

  if (write(controller->fd, &ie, sizeof(struct input_event)) < 0) {
    close_controller(controller);
    return -1;
  }
  if (emit_syn > 0)
    return emit(controller, EV_SYN, SYN_REPORT, 0, 0);
  return 0;
}

int emit_gas(simulation_controller_t *controller, uint8_t val) {
  if (emit(controller, EV_ABS, ABS_GAS, val, 1) < 0){ 
    LOG_DEBUG("Failed to emit ABS_GAS value %d.", val);
    return -1;
  }
  return 0;
}

int emit_brake(simulation_controller_t *controller, uint8_t val) {
  if (emit(controller, EV_ABS, ABS_BRAKE, val, 1) < 0) {
    LOG_DEBUG("Failed to emit ABS_BRAKE value %d.", val);
    return -1;
  }
  return 0;
}

int emit_btn(simulation_controller_t *controller, int btn, uint8_t val) {
  if (emit(controller, EV_KEY, BTN_TRIGGER_HAPPY1 + btn, val, 1) < 0) {
    LOG_DEBUG("Failed to emit value %d for btn %d.", val, btn);
    return -1;
  }
}

int emit_wheel(simulation_controller_t *controller, int16_t val) {
  if (emit(controller, EV_ABS, ABS_WHEEL, val, 1) < 0) {
    LOG_DEBUG("Failed to emit ABS_WHEEL value %d.", val);
    return -1;
  }
  return 0;
}

int get_controller_permit(simulation_controller_t *controller) {
  LOG_DEBUG("Getting wheel fd from uinput");
  // Obtain the fd for the wheel
  controller->fd = open("/dev/uinput", O_RDWR);
  if (controller->fd <= 0) {
    LOG_ERROR("Couldn't open /dev/uinput");
    return -1;
  }
  LOG_DEBUG("Done getting wheel fd from uinput");
  return 0;
}

int construct_controller(simulation_controller_t *controller) {
  if (add_wheel_w_pedals(controller) < 0) return -1;
  return 0;
}

int register_controller(simulation_controller_t *controller) {
  LOG_DEBUG("Registering wheel");
  struct uinput_user_dev udev;
  memset(&udev, 0, sizeof(struct uinput_user_dev));

  // Fill info
  udev.id.bustype = BUS_GAMEPORT;
  udev.ff_effects_max = 0;
  udev.absmin[ABS_WHEEL] = WHEEL_MIN_VALUE;
  udev.absmax[ABS_WHEEL] = WHEEL_MAX_VALUE;
  udev.absmin[ABS_GAS] = GAS_MIN_VALUE;
  udev.absmax[ABS_GAS] = GAS_MAX_VALUE;
  udev.absmin[ABS_BRAKE] = BRAKE_MIN_VALUE;
  udev.absmax[ABS_BRAKE] = BRAKE_MAX_VALUE;
  strcpy(udev.name, controller->name);

  // Register
  if (write(controller->fd, &udev, sizeof(struct uinput_user_dev)) < 0) {
    return -1;
  }
  LOG_DEBUG("Done registering wheel");
  return 0;
}

int confirm_controller(simulation_controller_t *controller) {
  LOG_DEBUG("Actually ask uinput to create the wheel");
  // Actually tell uinput to create the device
  if (ioctl(controller->fd, UI_DEV_CREATE) < 0) {
    return -1;
  }
  LOG_DEBUG("uinput said it has now created the wheel");
  return 0;
}

int setup_controller(simulation_controller_t *controller) {
  LOG_DEBUG("Setting up the virtual wheel");
  if (get_controller_permit(controller) < 0) {
    return -1;
  }

  if (construct_controller(controller) < 0) {
    return -1;
  }

  if (register_controller(controller) < 0) {
    return -1;
  }

  if (confirm_controller(controller) < 0) {
    return -1;
  }
  LOG_DEBUG("Done setting up the virtual wheel");
  return 0;
}

int remove_controller(simulation_controller_t *controller) {
  LOG_DEBUG("Removing the virtual wheel...");
  int res = 0;
  res = ioctl(controller->fd, UI_DEV_DESTROY);
  res |= close_controller(controller);
  free(controller);
  LOG_DEBUG("Removed.");
  return res;
}