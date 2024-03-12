#include <stdio.h>
#include "../../hal/include/joystick_gpioPress.h"
#include "../include/audioMixer_upd.h"

int terminate_flag = 0;

void UDP_Server() 
{
    UDP_initServer(&terminate_flag);
    UDP_join();
    UDP_cleanup();
}

int main() {
    int exported = isExported_gpioPress();
    printf("is exported: %d\n", exported);

    int gpio27_value = readValue_gpioPress();
    printf("value from gpio27: %d\n", gpio27_value);

    return 0;
}
