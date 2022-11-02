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
#define SIG_ROM_OUTPUT_ENABLE   10
#define SIG_ROM_CHIP_ENABLE     11
#define SIG_ROM_PROGRAM         12


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
    pinMode(SIG_ROM_OUTPUT_ENABLE, INPUT);
    pinMode(SIG_ROM_CHIP_ENABLE, INPUT);
    pinMode(SIG_ROM_PROGRAM, INPUT);

    pinMode(SIG_CLOCK_ENABLE, OUTPUT);
    pinMode(SIG_CLOCK_ALT, OUTPUT);

    digitalWrite(SIG_CLOCK_ENABLE, LOW);
    digitalWrite(SIG_CLOCK_ALT, LOW);
}

void C65X::enable_system_clock(bool enable) {
    if (enable) {
        digitalWrite(SIG_CLOCK_ENABLE, LOW);
    } else {
        digitalWrite(SIG_CLOCK_ENABLE, HIGH);
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
        digitalWrite(SIG_BUS_ENABLE, HIGH);
    } else {
        pinMode(SIG_BUS_ENABLE, OUTPUT);
        digitalWrite(SIG_BUS_ENABLE, LOW);
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
        WRITE_ADDR_H = 0xff;
        WRITE_ADDR_L = 0xff;

        DDR_DATA = B00000000;

        digitalWrite(SIG_READ_WRITE, HIGH);
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
    digitalWrite(SIG_READ_WRITE, HIGH);

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
    digitalWrite(SIG_READ_WRITE, LOW);
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

    digitalWrite(SIG_READ_WRITE, HIGH);
    DDR_DATA = B00000000;
}

void C65X::start_programming() {
    C65X::control_bus(true);

    digitalWrite(SIG_ROM_WRITE_ENABLE, HIGH);
    digitalWrite(SIG_ROM_CHIP_ENABLE, HIGH);
    digitalWrite(SIG_ROM_OUTPUT_ENABLE, HIGH);
    digitalWrite(SIG_ROM_PROGRAM, LOW);
    pinMode(SIG_ROM_OUTPUT_ENABLE, OUTPUT);
    pinMode(SIG_ROM_CHIP_ENABLE, OUTPUT);
    pinMode(SIG_ROM_WRITE_ENABLE, OUTPUT);
    pinMode(SIG_ROM_PROGRAM, OUTPUT);

}

void C65X::end_programming() {
    digitalWrite(SIG_ROM_WRITE_ENABLE, HIGH);
    digitalWrite(SIG_ROM_CHIP_ENABLE, HIGH);
    digitalWrite(SIG_ROM_PROGRAM, HIGH);
    digitalWrite(SIG_ROM_OUTPUT_ENABLE, HIGH);
    pinMode(SIG_ROM_WRITE_ENABLE, INPUT);
    pinMode(SIG_ROM_CHIP_ENABLE, INPUT);
    pinMode(SIG_ROM_OUTPUT_ENABLE, INPUT);
    pinMode(SIG_ROM_PROGRAM, INPUT);

    C65X::control_bus(false);
}

bool poll_byte(byte data) {
    byte poll = data ^ 0xff;
    int cycle = 0;
    while (data != poll && cycle++ < 100) {
        digitalWrite(SIG_ROM_OUTPUT_ENABLE, LOW);
        digitalWrite(SIG_ROM_CHIP_ENABLE, LOW);
        __asm__("nop\n\t");
        __asm__("nop\n\t");
        __asm__("nop\n\t");

        poll = READ_DATA;

        digitalWrite(SIG_ROM_OUTPUT_ENABLE, HIGH);
        digitalWrite(SIG_ROM_CHIP_ENABLE, HIGH);

        __asm__("nop\n\t");
        __asm__("nop\n\t");
        Serial.println(poll);
    }

    Serial.println(cycle);
    digitalWrite(SIG_ROM_OUTPUT_ENABLE, HIGH);
    digitalWrite(SIG_ROM_CHIP_ENABLE, HIGH);
    if (cycle >= 100) {
        Serial.println("MWrite failed!");
        return false;
    }
    Serial.println("MWrite okay");
    return true;
}

bool C65X::program_byte(word address, byte data) {
    digitalWrite(SIG_ROM_WRITE_ENABLE, HIGH);
    digitalWrite(SIG_ROM_CHIP_ENABLE, HIGH);
    digitalWrite(SIG_ROM_OUTPUT_ENABLE, HIGH);

    DDR_ADDR_H = B11111111;
    DDR_ADDR_L = B11111111;
    DDR_DATA = B11111111;
    WRITE_ADDR_H = highByte(address);
    WRITE_ADDR_L = lowByte(address);
    WRITE_DATA = data;

    __asm__("nop\n\t");
    digitalWrite(SIG_ROM_CHIP_ENABLE, LOW);
    __asm__("nop\n\t");

    digitalWrite(SIG_ROM_WRITE_ENABLE, LOW);
    __asm__("nop\n\t");
    __asm__("nop\n\t");
    __asm__("nop\n\t");
    digitalWrite(SIG_ROM_WRITE_ENABLE, HIGH);
    __asm__("nop\n\t");
    __asm__("nop\n\t");

    DDR_DATA = B00000000;
    digitalWrite(SIG_ROM_OUTPUT_ENABLE, LOW);

    bool result = poll_byte(data);

    return result;
}

