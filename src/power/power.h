#ifndef POWER_H
#define POWER_H

#include <stdint.h>

// Try to power off the machine via common ACPI/io ports.
void power_off(void);

#endif
