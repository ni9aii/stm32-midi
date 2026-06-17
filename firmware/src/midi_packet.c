#include "midi_packet.h"

#define MIDI_CABLE_NUMBER 0u
#define MIDI_CIN_NOTE_OFF 0x8u
#define MIDI_CIN_NOTE_ON 0x9u
#define MIDI_CHANNEL_1 0x0u

static midi_packet_t make_event(uint8_t cin, uint8_t status, uint8_t note,
                                uint8_t velocity) {
  midi_packet_t packet;

  packet.bytes[0] = (uint8_t)((MIDI_CABLE_NUMBER << 4) | cin);
  packet.bytes[1] = (uint8_t)(status | MIDI_CHANNEL_1);
  packet.bytes[2] = note;
  packet.bytes[3] = velocity;

  return packet;
}

midi_packet_t midi_packet_note_on(uint8_t note, uint8_t velocity) {
  return make_event((uint8_t)MIDI_CIN_NOTE_ON, 0x90u, note, velocity);
}

midi_packet_t midi_packet_note_off(uint8_t note, uint8_t velocity) {
  return make_event((uint8_t)MIDI_CIN_NOTE_OFF, 0x80u, note, velocity);
}
