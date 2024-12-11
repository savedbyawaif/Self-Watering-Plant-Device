
#include "defines.h"
#include "lcd.h"
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#define OC1A PB1
#define MTR PD6

volatile uint8_t timer_cycle = 0;
volatile uint16_t water_level;
volatile uint16_t moisture;

volatile uint8_t ADC_L; // low register ADC
volatile uint8_t ADC_H; // high register

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

// ADC conversion interrupt, read ADC values
ISR(ADC_vect){
    ADC_L = ADCL;
    ADC_H = ADCH;
    if ((ADMUX & 0x0F) == 1) { // Connect to Water Level Sensor ADC1
        water_level = ((uint16_t)ADC_H << 8) | ADC_L;
    } 
    else if ((ADMUX & 0x0F) == 2) { // Moisture Sensor ADC2
        moisture = ((uint16_t)ADC_H << 8) | ADC_L;
    }
}

// Uses Pin OC1A to generate Fast PWM
void buzzer_init(void) {
    TCCR1A = (1 << WGM11); // Clear OC1A on compare match
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); // Fast PWM mode
    ICR1 = 2200; // TOP value
    OCR1A = ICR1/2; // 50% of TOP value
}

// Turns buzzer on
void buzzer_on (void) {
    TCCR1A |= (1 << COM1A1); // Connect OC1A
    _delay_ms(50); 
}

// Turns Buzzer off
void buzzer_off(void) {
    TCCR1A &= ~(1 << COM1A1); // Disconnect OC1A
    PORTB &= ~(1 << OC1A);  // Set OC1A to LOW
}

// To limit current draw, phase correct PWM signal controls motor
void motor_PWM_init(void) {
    TCCR0A = (1 << COM0A1) | (1 << WGM00); // Phase Correct PWM
    TCCR0B = (1 << CS01); // Pre-scaler = 8
    OCR0A = 140; // 55% duty cycle
}

// Connect Motor
void enable_PWM(void) {
    TCCR0A |= (1 << COM0A1); // Connect OC0A
}

// Disable Motor
void disable_PWM(void) {
    TCCR0A &= ~(1 << COM0A1); // Disconnect OC0A
}

void adc_init(void) {
    //Sets AVcc as reference voltage
    ADMUX |= (1 << REFS0);
    ADMUX &= ~(1 << REFS1);

    // Enable ADC and sets pre-scaler to 16
    ADCSRA |= (1 << ADEN) | (1 << ADPS2);
    ADCSRA |= (1 << ADIE);
}

int main(void) {
    // Register Initialization
    PORTD &= ~(1 << MTR); // Ensure off

    DDRD |= (1 << PD7); // RED
    DDRB |= (1 << PB0); // GREEN
    DDRB |= (1 << OC1A); // PB1 OC1A PWM pin 
    DDRD |= (1<< MTR); // PD6 Motor Control PWM pin
   
    // Initialization of registers
    buzzer_init();
    motor_PWM_init(); 
    adc_init(); 
    lcd_init(); 
    
    sei(); // Enable global interrupts
    
    while (1) {
        // Clear ADC register and receive ADC1 input signal
        ADMUX = (ADMUX & ~(0x0F)) | (1 << MUX0);
        ADCSRA |= (1 << ADSC);
        while ((ADCSRA & (1 << ADSC))) {} // polling to continue
        
        // Clear ADC register and receive ADC2 input signal
        ADMUX = (ADMUX & ~(0x0F)) | (1 << MUX1);
        ADCSRA |= (1 << ADSC);
        while ((ADCSRA & (1 << ADSC))) {} // polling to continue

        // Display Values on LCD
        fprintf(&lcd_str, "\x1b\x01");
        fprintf(&lcd_str, "water level =%u%% \x1b\xc0moisture=%u%% ", water_level/7, 100-(moisture-180)/3);

        // Water Level Sensor Condition, Buzz when below analog threshold
        if (water_level < 150){ buzzer_on(); }
        else { buzzer_off(); }
 
        // Moisture Sensor Condition, Turn motor on above analog threshold
        if (moisture > 400) {
            PORTD |= (1<<PD7); // RED on
            PORTB &= ~(1<<PB0); // GREEN off
            // Track timer cycles for periodic "on" durations
            if (timer_cycle == 50) { // disabled until condition
                enable_PWM();
            }
            else if (timer_cycle == 70) { // enabled until condition
                disable_PWM();
                timer_cycle = 0;
            }
            timer_cycle++;
        }
        // Turn off motor for higher moisture
        else if ((moisture > 300) && (moisture < 400)) {
            PORTD |= (1<<PD7); // RED on
            PORTB |= (1<<PB0); // GREEN on
            disable_PWM();
        }
        else if(moisture < 300) {
            PORTB |= (1<<PB0); // GREEN on
            PORTD &=~ (1<<PD7); // RED off
            disable_PWM();
        }
        _delay_us(100000); // Slow down refresh rate
    }
    
    return 0;
}


