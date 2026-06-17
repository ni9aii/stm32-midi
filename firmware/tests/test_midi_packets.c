#include "midi_packet.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void expect_packet(midi_packet_t actual, const uint8_t expected[4]) {
  assert(sizeof(actual.bytes) == MIDI_PACKET_SIZE);
  assert(memcmp(actual.bytes, expected, MIDI_PACKET_SIZE) == 0);
}

int main(void) {
  expect_packet(midi_packet_note_on(60, 100),
                (const uint8_t[]){0x09, 0x90, 60, 100});
  expect_packet(midi_packet_note_off(60, 0),
                (const uint8_t[]){0x08, 0x80, 60, 0});

  return 0;
}
