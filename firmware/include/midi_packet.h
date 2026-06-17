#ifndef MIDI_PACKET_H
#define MIDI_PACKET_H

#include <stdint.h>

#define MIDI_PACKET_SIZE 4u

typedef struct {
  uint8_t bytes[MIDI_PACKET_SIZE];
} midi_packet_t;

midi_packet_t midi_packet_note_on(uint8_t note, uint8_t velocity);
midi_packet_t midi_packet_note_off(uint8_t note, uint8_t velocity);

#endif
