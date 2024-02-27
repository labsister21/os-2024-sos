#include "header/driver/keyboard.h"
#include "header/cpu/interrupt.h"
#include "header/cpu/portio.h"
#include "header/text/buffercolor.h"
#include "header/text/framebuffer.h"
#include <stdint.h>

const char keyboard_scancode_1_to_ascii_map[256] = {
    0,   0x1B, '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,

    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,
};

const char keyboard_scancode_1_to_ascii_map_shifted[256] = {
    0,   0x1B, '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '"',  '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,

    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,
};

struct KeyboardDriverState keyboard_state = {
    .keyboard_input_on = false,
    .keyboard_buffer = '\0',
    .shift_modifier = false,
    .alt_modifier = false,
    .ctrl_modifier = false
};

void keyboard_state_activate() { keyboard_state.keyboard_input_on = true; }

void keyboard_state_deactivate() { keyboard_state.keyboard_input_on = false; }

void get_keyboard_buffer(char *buff) {
  *buff = keyboard_state.keyboard_buffer;
  keyboard_state.keyboard_buffer = '\0';
}

void keyboard_isr() {
  if (!keyboard_state.keyboard_input_on) return;
  uint8_t scancode = in(KEYBOARD_DATA_PORT);
  char c;
  if (keyboard_state.shift_modifier)
    c = keyboard_scancode_1_to_ascii_map_shifted[scancode];
  else c = keyboard_scancode_1_to_ascii_map[scancode];
  keyboard_state.keyboard_buffer = c;

  switch (scancode) {

  case EXT_SCANCODE_RIGHT_SHIFT + MAKE_OFFSET:
  case EXT_SCANCODE_LEFT_SHIFT + MAKE_OFFSET:
    keyboard_state.shift_modifier = true;
    break;

  case EXT_SCANCODE_ALT + MAKE_OFFSET:
    keyboard_state.shift_modifier = true;
    break;

  case EXT_SCANCODE_CTRL + MAKE_OFFSET:
    keyboard_state.ctrl_modifier = true;
    break;

  case EXT_SCANCODE_RIGHT_SHIFT + BREAK_OFFSET:
  case EXT_SCANCODE_LEFT_SHIFT + BREAK_OFFSET:
    keyboard_state.shift_modifier = false;
    break;

  case EXT_SCANCODE_ALT + BREAK_OFFSET:
    keyboard_state.shift_modifier = false;
    break;

  case EXT_SCANCODE_CTRL + BREAK_OFFSET:
    keyboard_state.ctrl_modifier = false;
    break;
  }

  // if (!(scancode & 0x80)) {
  //   // framebuffer_clear();
  //   int m = 100;
  //   int col = 0;
  //   while (m > 0) {
  //     framebuffer_write(0, col++, (scancode / m) + '0', WHITE, BLACK);
  //     scancode = scancode % m;
  //     m = m / 10;
  //   }
  // }

  pic_ack(PIC1_OFFSET + IRQ_KEYBOARD);
}
