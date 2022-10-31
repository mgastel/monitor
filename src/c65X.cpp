//
// Created by Mark Gastel on 2022-10-29.
//
#include <arduino.h>
#include <pins_arduino.h>
#include "c65X.h"

#define DDR_ADDR_H  DDRC
#define DDR_ADDR_L  DDRL
#define DDR_DATA    DDRA
#define READ_ADDR_H  PINC
#define READ_ADDR_L  PINL
#define READ_DATA    PINA
#define WRITE_ADDR_H  PORTC
#define WRITE_ADDR_L  PORTL
#define WRITE_DATA    PORTA

#define SIG_CLOCK               2
#define SIG_READ_WRITE          3
#define SIG_CLOCK_ENABLE        4
#define SIG_CLOCK_ALT           5
#define SIG_SYNC                6
#define SIG_VPB                 7
#define SIG_BUS_ENABLE          8
#define SIG_ROM_WRITE_ENABLE    9


C65X::C65X() {}

void C65X::initialize() {
    C65X::break_point = 0;

    // set data and address bus to input
    DDR_DATA = 0;
    DDR_ADDR_L = 0;
    DDR_ADDR_H = 0;

    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(SIG_CLOCK, INPUT);
    pinMode(SIG_READ_WRITE, INPUT);
    pinMode(SIG_SYNC, INPUT);
    pinMode(SIG_VPB, INPUT);
    pinMode(SIG_BUS_ENABLE, INPUT);
    pinMode(SIG_ROM_WRITE_ENABLE, INPUT);

    pinMode(SIG_CLOCK_ENABLE, OUTPUT);
    pinMode(SIG_CLOCK_ALT, OUTPUT);

    digitalWrite(SIG_CLOCK_ENABLE, 1);
    digitalWrite(SIG_CLOCK_ALT, 0);
}

void C65X::enable_system_clock(bool enable) {
    if (enable) {
        digitalWrite(SIG_CLOCK_ENABLE, 1);
    } else {
        digitalWrite(SIG_CLOCK_ENABLE, 0);
    }
}

void clock(int signal) {
    digitalWrite(SIG_CLOCK_ALT, signal);
    digitalWrite(LED_BUILTIN, signal);
}

void C65X::clock_cycles(int cycles) {
    int i;

    for (i = 0; i < cycles * 2; i++) {
        clock(1);
        __asm__("nop\n\t");
        __asm__("nop\n\t");
        clock(0);
        __asm__("nop\n\t");
        __asm__("nop\n\t");
    }
}

void C65X::bus_enable(bool enable) {
    if (enable) {
        pinMode(SIG_BUS_ENABLE, INPUT);
        digitalWrite(SIG_BUS_ENABLE, 1);
    } else {
        pinMode(SIG_BUS_ENABLE, OUTPUT);
        digitalWrite(SIG_BUS_ENABLE, 0);
    }
}

void C65X::set_break_point(word address) {
    C65X::break_point = address;
}

void C65X::clear_break_point() {
    C65X::break_point = 0;
}

bool C65X::step() {
    char buffer[32];

    clock(0);
    __asm__("nop\n\t");

    clock(1);

    __asm__("nop\n\t");

    char flag = ' ';
    if (digitalRead(SIG_SYNC)) {
        flag = 'S';
    } else if (! digitalRead(SIG_VPB)) {
        flag = 'V';
    }
    char rw = digitalRead(SIG_READ_WRITE)? 'r':'W';

    word address = ((word) READ_ADDR_H) << 8 | READ_ADDR_L;
    sprintf(buffer, "T%c %04x %c %02x", flag, (unsigned int) address, rw, (unsigned int) READ_DATA);
    Serial.println(buffer);

    clock(0);

    if (C65X::break_point != 0 && C65X::break_point == address) {
        return true;
    }
    return false;
}

void C65X::control_bus(bool enable) {
    if (enable) {
        C65X::bus_enable(false);
        C65X::enable_system_clock(false);
        C65X::clock_cycles(10);

        DDR_ADDR_H = B11111111;
        DDR_ADDR_L = B11111111;
        WRITE_ADDR_H = 0;
        WRITE_ADDR_L = 0;

        DDR_DATA = B00000000;

        digitalWrite(SIG_READ_WRITE, 1);
        pinMode(SIG_READ_WRITE, OUTPUT);
    } else {
        DDR_ADDR_H = B00000000;
        DDR_ADDR_L = B00000000;

        DDR_DATA = B00000000;

        pinMode(SIG_READ_WRITE, INPUT);
        C65X::bus_enable(true);
    }
}

void C65X::read_address(word address) {
    char buffer[16];

    clock(0);
    digitalWrite(SIG_READ_WRITE, 1);

    WRITE_ADDR_H = highByte(address);
    WRITE_ADDR_L =  lowByte(address);

    __asm__("nop\n\t");
    clock(1);
    __asm__("nop\n\t");
    __asm__("nop\n\t");
    byte data = READ_DATA;
    clock( 0);

    sprintf(buffer, "R%04x %02x", (unsigned int) address, (unsigned int) data);
    Serial.println(buffer);
}

void  C65X::write_address(word address, byte data) {
    clock( 0);
    digitalWrite(SIG_READ_WRITE, 0);
    WRITE_ADDR_H = highByte(address);
    WRITE_ADDR_L = lowByte(address);

    DDR_DATA = B11111111;
    WRITE_DATA = data;
    __asm__("nop\n\t");
    __asm__("nop\n\t");
    clock(1);
    __asm__("nop\n\t");
    __asm__("nop\n\t");
    clock(0);
    __asm__("nop\n\t");
    __asm__("nop\n\t");

    digitalWrite(SIG_READ_WRITE, 1);
    DDR_DATA = B00000000;
}

void C65X::start_programming() {

}

void C65X::end_programming() {

}

bool C65X::program_byte(word address, byte data) {
    return false;
}
