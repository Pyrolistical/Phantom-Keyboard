/* USB Keyboard Firmware code for the Phantom Keybaord
 * http://geekhack.org/showwiki.php?title=Island:26742
 * Copyright (c) 2012 Fredrik Atmer, Bathroom Epiphanies Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard.h"

#define bool            uint8_t
#define true            1
#define false           0
#define NULL            0
#define NA              0

#define _DDRB           (uint8_t *const)&DDRB
#define _DDRC           (uint8_t *const)&DDRC
#define _DDRD           (uint8_t *const)&DDRD
#define _DDRE           (uint8_t *const)&DDRE
#define _DDRF           (uint8_t *const)&DDRF

#define _PINB           (uint8_t *const)&PINB
#define _PINC           (uint8_t *const)&PINC
#define _PIND           (uint8_t *const)&PIND
#define _PINE           (uint8_t *const)&PINE
#define _PINF           (uint8_t *const)&PINF

#define _PORTB          (uint8_t *const)&PORTB
#define _PORTC          (uint8_t *const)&PORTC
#define _PORTD          (uint8_t *const)&PORTD
#define _PORTE          (uint8_t *const)&PORTE
#define _PORTF          (uint8_t *const)&PORTF

#define _PIN0 0x01
#define _PIN1 0x02
#define _PIN2 0x04
#define _PIN3 0x08
#define _PIN4 0x10
#define _PIN5 0x20
#define _PIN6 0x40
#define _PIN7 0x80

/* NROW number of rows
   NCOL number of columns
   NKEY = NROW*NCOL */
#define NROW            6
#define NCOL            17
#define NKEY            102

/* Modifier keys are handled differently and need to be identified */
const uint8_t is_modifier[NKEY] = {
  true,            true,            false,           false,           false,           false,  // COL  0
  true,            false,           false,           false,           false,           false,  // COL  1
  true,            false,           false,           false,           false,           false,  // COL  2
  NA,              false,           false,           false,           false,           false,  // COL  3
  NA,              false,           false,           false,           false,           false,  // COL  4
  NA,              false,           false,           false,           false,           false,  // COL  5
  NA,              false,           false,           false,           false,           false,  // COL  6
  false,           false,           false,           false,           false,           false,  // COL  7
  NA,              false,           false,           false,           false,           false,  // COL  8
  NA,              false,           false,           false,           false,           false,  // COL  9
  true,            false,           false,           false,           false,           false,  // COL 10
  true,            false,           false,           false,           false,           false,  // COL 11
  false,           NA,              false,           false,           NA,              false,  // COL 12
  true,            true,            false,           false,           false,           false,  // COL 13

  false,           NA,              NA,              false,           false,           false,  // COL 14
  false,           false,           NA,              false,           false,           false,  // COL 15
  false,           NA,              NA,              false,           false,           false,  // COL 16
};

const uint8_t layout[NKEY] = {
//ROW 0            ROW 1            ROW 2            ROW 3            ROW 4
  KEY_LEFT_CTRL,   KEY_LEFT_SHIFT,  KEY_CAPS_LOCK,   KEY_TAB,         KEY_1,              KEY_ESC,        // COL  0
  KEY_LEFT_GUI,    KEY_PIPE,        KEY_A,           KEY_Q,           KEY_2,              KEY_TILDE,      // COL  1
  KEY_LEFT_ALT,    KEY_Z,           KEY_S,           KEY_W,           KEY_3,              KEY_F1,         // COL  2
  NA,              KEY_X,           KEY_D,           KEY_E,           KEY_4,              KEY_F2,         // COL  3
  NA,              KEY_C,           KEY_F,           KEY_R,           KEY_5,              KEY_F3,         // COL  4
  NA,              KEY_V,           KEY_G,           KEY_T,           KEY_6,              KEY_F4,         // COL  5
  NA,              KEY_B,           KEY_H,           KEY_Y,           KEY_7,              KEY_F5,         // COL  6
  KEY_SPACE,       KEY_N,           KEY_J,           KEY_U,           KEY_8,              KEY_F6,         // COL  7
  NA,              KEY_M,           KEY_K,           KEY_I,           KEY_9,              KEY_F7,         // COL  8
  NA,              KEY_COMMA,       KEY_L,           KEY_O,           KEY_0,              KEY_F8,         // COL  9
  KEY_RIGHT_ALT,   KEY_PERIOD,      KEY_SEMICOLON,   KEY_P,           KEY_MINUS,          KEY_F9,         // COL 10
  KEY_RIGHT_GUI,   KEY_SLASH,       KEY_QUOTE,       KEY_LEFT_BRACE,  KEY_EQUAL,          KEY_F10,        // COL 11
  KEY_APPLICATION, NA,              KEY_BACKSLASH,   KEY_RIGHT_BRACE, NA,                 KEY_F11,        // COL 12
  KEY_RIGHT_CTRL,  KEY_RIGHT_SHIFT, KEY_ENTER,       KEY_BACKSLASH,   KEY_BACKSPACE,      KEY_F12,        // COL 13

  KEY_LEFT,        NA,              NA,              KEY_DELETE,      KEY_INSERT,         KEY_PRINTSCREEN,// COL 14
  KEY_DOWN,        KEY_UP,          NA,              KEY_END,         KEY_HOME,           KEY_SCROLL_LOCK,// COL 15
  KEY_RIGHT,       NA,              NA,              KEY_PAGE_DOWN,   KEY_PAGE_UP,        KEY_PAUSE,      // COL 16
};

/* Specifies the ports and pin numbers for the rows */
uint8_t *const  row_ddr[NROW] = { _DDRB,  _DDRB,  _DDRB,  _DDRB,  _DDRB,  _DDRB};
uint8_t *const row_pull[NROW] = {_PORTB, _PORTB, _PORTB, _PORTB, _PORTB, _PORTB};
uint8_t *const row_port[NROW] = { _PINB,  _PINB,  _PINB,  _PINB,  _PINB,  _PINB};
const uint8_t   row_bit[NROW] = { _PIN0,  _PIN1,  _PIN2,  _PIN3,  _PIN4,  _PIN5};

/* Specifies the ports and pin numbers for the columns */
uint8_t *const  col_ddr[NCOL] = { _DDRD,  _DDRC,  _DDRC,  _DDRD,  _DDRD,  _DDRE,
				  _DDRF,  _DDRF,  _DDRF,  _DDRF,  _DDRF,  _DDRF,
				  _DDRD,  _DDRD,  _DDRD,  _DDRD,  _DDRD};
uint8_t *const col_port[NCOL] = {_PORTD, _PORTC, _PORTC, _PORTD, _PORTD, _PORTE,
				 _PORTF, _PORTF, _PORTF, _PORTF, _PORTF, _PORTF,
				 _PORTD, _PORTD, _PORTD, _PORTD, _PORTD};
const uint8_t   col_bit[NCOL] = { _PIN1,  _PIN7,  _PIN6,  _PIN4,  _PIN0,  _PIN6,
				  _PIN0,  _PIN1,  _PIN4,  _PIN1,  _PIN6,  _PIN7,
				  _PIN7,  _PIN6,  _PIN1,  _PIN2,  _PIN3};

/* pressed  keeps track of which keys that are pressed
   queue    contains the keys that are sent in the HID packet
   mod_keys is the bit pattern corresponding to pressed modifier keys */
bool pressed[NKEY];
uint8_t queue[7] = {255,255,255,255,255,255,255};
uint8_t mod_keys = 0;

void init(void);
void send(void);
void key_press(uint8_t key_id);
void key_release(uint8_t key_id);

int main(void) {
  uint8_t row, col, key_id;

  init();

  for(;;) {
    _delay_ms(5);                                //  Debouncing
    for(col=0; col<NCOL; col++) {
      *col_port[col] &= ~col_bit[col];
      _delay_us(1);
      for(row=0; row<NROW; row++) {
	key_id = col*NROW+row;
	if(!(*row_port[row] & row_bit[row])) {
	  if(!pressed[key_id])
	    key_press(key_id);
	}
	else if(pressed[key_id])
	  key_release(key_id);
      }
      *col_port[col] |= col_bit[col];
    }

    //    OCR1B++; OCR1C++;

    // TODO fixed keyboard leds.  I disabled as I cannot test them
    //PORTB = (PORTB & 0b00111111) | ((keyboard_leds << 5) & 0b11000000);
    //DDRB  = (DDRB  & 0b00111111) | ((keyboard_leds << 5) & 0b11000000);

  }
}

inline void send(void) {
  //return;
  uint8_t i;
  for(i=0; i<6; i++)
    keyboard_keys[i] = queue[i]<255? layout[queue[i]]: 0;
  keyboard_modifier_keys = mod_keys;
  usb_keyboard_send();
}

inline void key_press(uint8_t key_id) {
  uint8_t i;
  pressed[key_id] = true;
  if(is_modifier[key_id])
    mod_keys |= layout[key_id];
  else {
    for(i=5; i>0; i--) queue[i] = queue[i-1];
    queue[0] = key_id;
  }
  send();
}

inline void key_release(uint8_t key_id) {
  uint8_t i;
  pressed[key_id] = false;
  if(is_modifier[key_id])
    mod_keys &= ~layout[key_id];
  else {
    for(i=0; i<6; i++) if(queue[i]==key_id) break;
    for(; i<6; i++) queue[i] = queue[i+1];
  }
  send();
}

void init(void) {
  uint8_t i;
  CLKPR = 0x80; CLKPR = 0;
  usb_init();
  while(!usb_configured());
  _delay_ms(1000);
  // init rows for input
  for(uint8_t row=0; row<NROW; row++) {
    *row_ddr[row] &= ~row_bit[row];
    *row_pull[row] |= row_bit[row];
  }
  // init cols for output
  for(uint8_t col=0; col<NCOL; col++) {
    *col_ddr[col] |= col_bit[col];
    *col_port[col] |= col_bit[col];
  }
  for(i=0; i<NKEY; i++) pressed[i] = false;

  // TODO fixed keyboard leds.  I disabled as I cannot test them
  // LEDs are on output compare pins OC1B OC1C
  // This activates fast PWM mode on them.
  // OCR1B sets the intensity
  //TCCR1A = 0b00101001;
  //TCCR1B = 0b00001001;
  //OCR1B = OCR1C = 32;

  // LEDs: LED_A -> PORTB6, LED_B -> PORTB7
  //DDRB  &= 0b00000000;
  //PORTB &= 0b00111111;
}
