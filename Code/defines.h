/* CPU frequency */
#define F_CPU 14745600UL
/* UART baud rate */
#define UART_BAUD 9600
/* HD44780 LCD port connections */
#define HD44780_RS C, 3
#define HD44780_RW C, 4
#define HD44780_E C, 5
/* The data bits have to be not only in ascending order but also consecutive. */
#define HD44780_D4 D, 2
/* Whether to read the busy flag, or fall back to worst-time delays. */
#define USE_BUSY_BIT 1