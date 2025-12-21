#include "power.h"
#include "../clib/clib.h"

// Attempt several common port-based soft-off methods used by VMs / firmwares.
// This is best-effort — proper ACPI shutdown requires parsing FADT and ACPI tables.
void power_off(void) {
    // QEMU / some machines: write to port 0x604
    outw(0x604, 0x2000);
    // Some other VMs / boards respond to 0xB004
    outw(0xB004, 0x2000);
    // Last-ditch: try 0x4004
    outw(0x4004, 0x2000);
}
