/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, upper layer of LCD driver.
 *
 * $Id: lcd.c 2265 2011-12-28 21:28:29Z joerg_wunsch $
 */

#include "defines.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "hd44780.h"
#include "lcd.h"

/*
 * Setup the LCD controller.  First, call the hardware initialization
 * function, then adjust the display attributes we want.
 */
void
lcd_init(void)
{

  hd44780_init();

  /*
   * Clear the display.
   */
  hd44780_outcmd(HD44780_CLR);
  hd44780_wait_ready(true);

  /*
   * Entry mode: auto-increment address counter, no display shift in
   * effect.
   */
  hd44780_outcmd(HD44780_ENTMODE(1, 0));
  hd44780_wait_ready(false);

  /*
   * Enable display, activate non-blinking cursor.
   */
  hd44780_outcmd(HD44780_DISPCTL(1, 1, 0));
  hd44780_wait_ready(false);
}
/*
 * Send character c to the LCD display.  After a an ESC (0x1B) has been seen,
 * the next character (byte) will be sent to the LCD as a command.
 */
int
lcd_putchar(char c, FILE *unused) {
  static bool esc_seen;   // Use of the static keyword makes the value persistent.

  if (!esc_seen) {
      /* The last character was not an escape character. Check for this time. */
      if (c == 0x1b) {
        esc_seen = true;
      } else {
        /* The information can be written as data. */
        hd44780_wait_ready(false);
        hd44780_outdata(c);
      }
  } else {
      /* Last character was ESC = 0x1B: treat this byte as a command. Since the command 
         is arbitrary it may be one that takes a long time to execute: "Clear Display" 
         and "Return Home" are those commands.  However, if USE_BUSY_BIT is true, then a
         spin-wait on BF is made. */
      hd44780_wait_ready(false);
      hd44780_outcmd(c); // Send the byte as a command
      
      if (c <= 3) {
        /* This is either a Clear Display or Return home command. */
        hd44780_wait_ready(true);  // Long delay.
      }
      
      /* Clear the flag to return to "normal" data processing. */
      esc_seen = false;
  }
  
  return 0;
}
