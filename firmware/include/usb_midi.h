#ifndef USB_MIDI_H
#define USB_MIDI_H

#include <stdbool.h>
#include <stdint.h>

bool usb_midi_init(void);
bool usb_midi_connected(void);
void usb_midi_poll(void);
bool usb_midi_note_on(uint8_t note, uint8_t velocity);
bool usb_midi_note_off(uint8_t note, uint8_t velocity);

#endif
