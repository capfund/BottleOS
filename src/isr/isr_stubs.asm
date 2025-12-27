[BITS 32]
global isr_stub_table
extern isr_common_stub

; Table of 256 ISR entry points
isr_stub_table:
    times 256 dd isr_common_stub
