//
// Created by Mark Gastel on 2022-10-29.
//
#include <arduino.h>
#include <pins_arduino.h>
#include "c65X.h"

#define DDR_ADDR_H  DDRC
#define DDR_ADDR_L  DDRL
#define DDR_DATA    DDRA
#define WRITE_ADDR_H  PORTC
#define WRITE_ADDR_L  PORTL
#define WRITE_DATA    PORTA

#define BUS_READ_WRITE          3
#define BUS_CLOCK_ENABLE        4
#define BUS_CLOCK_ALT           5
#define BUS_SYNC                6
#define BUS_VPB                 7
#define BUS_BUS_ENABLE          8
#define BUS_ROM_WRITE_ENABLE    9
#define BUS_ROM_OUTPUT_ENABLE   10
#define BUS_ROM_CHIP_ENABLE     11
#define BUS_ROM_PROGRAM         12

#define bus_system_clock_active()        digitalWrite(BUS_CLOCK_ENABLE, LOW)
#define bus_system_clock_idle()          digitalWrite(BUS_CLOCK_ENABLE, HIGH)
#define bus_bus_enable_active()          digitalWrite(BUS_BUS_ENABLE, LOW)
#define bus_bus_enable_idle()            digitalWrite(BUS_BUS_ENABLE, HIGH)
#define bus_read_write_write()           digitalWrite(BUS_READ_WRITE, LOW)
#define bus_read_write_read()            digitalWrite(BUS_READ_WRITE, HIGH)
#define rom_write_active()               digitalWrite(BUS_ROM_WRITE_ENABLE, LOW)
#define rom_write_idle()                 digitalWrite(BUS_ROM_WRITE_ENABLE, HIGH)
#define rom_output_active()              digitalWrite(BUS_ROM_OUTPUT_ENABLE, LOW)
#define rom_output_idle()                digitalWrite(BUS_ROM_OUTPUT_ENABLE, HIGH)
#define rom_enable_active()              digitalWrite(BUS_ROM_CHIP_ENABLE, LOW)
#define rom_enable_idle()                digitalWrite(BUS_ROM_CHIP_ENABLE, HIGH)
#define rom_program_active()             digitalWrite(BUS_ROM_PROGRAM, LOW)
#define rom_program_idle()               digitalWrite(BUS_ROM_PROGRAM, HIGH)

#define bus_read_write_state_input()     pinMode(BUS_READ_WRITE, INPUT)
#define bus_read_write_state_output()    pinMode(BUS_READ_WRITE, OUTPUT)
#define bus_bus_enable_state_input()     pinMode(BUS_BUS_ENABLE, INPUT)
#define bus_bus_enable_state_output()    pinMode(BUS_BUS_ENABLE, OUTPUT)


#define bus_rom_state(X) { \
    pinMode(BUS_ROM_OUTPUT_ENABLE, X); \
    pinMode(BUS_ROM_CHIP_ENABLE, X); \
    pinMode(BUS_ROM_WRITE_ENABLE, X); \
    pinMode(BUS_ROM_PROGRAM, X); \
}

#define STATE_REG_OUTPUT B11111111
#define STATE_REG_INPUT  B00000000
#define bus_data_state_reg(X)   DDR_DATA = X
#define bus_address_state_reg(X) { \
    DDR_ADDR_H = X; \
    DDR_ADDR_L = X; \
}
#define get_bus_address() ((word)PINC << 8) | (word)PINL
#define get_bus_data()    PINA
#define set_bus_address(X) { \
    WRITE_ADDR_H = highByte(X); \
    WRITE_ADDR_L = lowByte(X); \
}
#define set_bus_address_low(X) { \
    WRITE_ADDR_L = X; \
}
#define set_bus_data(X) { \
    WRITE_DATA = X; \
}

#define delay_cycle()     __asm__("nop\n\t")


void clock(int signal) {
    digitalWrite(BUS_CLOCK_ALT, signal);
    digitalWrite(LED_BUILTIN, signal);
}



C65X::C65X() {}

void C65X::initialize() {
    C65X::break_point = 0;

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUS_SYNC, INPUT);
    pinMode(BUS_VPB, INPUT);
    pinMode(BUS_CLOCK_ENABLE, OUTPUT);
    pinMode(BUS_CLOCK_ALT, OUTPUT);

    bus_read_write_state_input();
    bus_bus_enable_state_input();
    bus_rom_state(INPUT);

    // set data and address bus to input
    bus_address_state_reg(STATE_REG_INPUT);
    bus_data_state_reg(STATE_REG_INPUT);

    C65X::enable_system_clock(true);
    clock(LOW);
}

void C65X::enable_system_clock(bool enable) {
    if (enable) {
        bus_system_clock_active();
    } else {
        bus_system_clock_idle();
    }
}


void C65X::clock_cycles(int cycles) {
    int i;

    for (i = 0; i < cycles * 2; i++) {
        clock(HIGH);
        delay_cycle();
        delay_cycle();
        clock(LOW);
        delay_cycle();
        delay_cycle();
    }
}

void C65X::bus_enable(bool enable) {
    if (enable) {
        bus_bus_enable_state_input();
        bus_bus_enable_idle();
    } else {
        bus_bus_enable_state_output();
        bus_bus_enable_active();
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
    delay_cycle();
    clock(1);
    delay_cycle();

    char flag = ' ';
    if (digitalRead(BUS_SYNC)) {
        flag = 'S';
    } else if (! digitalRead(BUS_VPB)) {
        flag = 'V';
    }
    char rw = digitalRead(BUS_READ_WRITE)? 'r':'W';

    word address = get_bus_address();
    sprintf(buffer, "T%c%04x%c%02x", flag, (unsigned int) address, rw, (unsigned int) get_bus_data());
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

        bus_address_state_reg(STATE_REG_OUTPUT);
        set_bus_address(0xffff);
        bus_data_state_reg(STATE_REG_INPUT);
        bus_read_write_read();
        bus_read_write_state_output();
    } else {
        bus_address_state_reg(STATE_REG_INPUT);
        bus_data_state_reg(STATE_REG_INPUT);
        bus_read_write_read();
        bus_read_write_state_input();

        C65X::bus_enable(true);
    }
}

void C65X::read_address(word address) {
    char buffer[16];

    clock(0);
    bus_read_write_read();

    set_bus_address(address);
    delay_cycle();
    clock(1);
    delay_cycle();
    delay_cycle();
    byte data = get_bus_data();
    clock( 0);

    sprintf(buffer, "R%04x%02x", (unsigned int) address, (unsigned int) data);
    Serial.println(buffer);
}

void  C65X::write_address(word address, byte data) {
    clock( 0);
    bus_read_write_write();
    set_bus_address(address);;

    bus_data_state_reg(STATE_REG_OUTPUT);
    set_bus_data(data);
    delay_cycle();
    delay_cycle();
    clock(1);
    delay_cycle();
    delay_cycle();
    clock(0);
    delay_cycle();
    delay_cycle();

    bus_read_write_read();
    bus_data_state_reg(STATE_REG_INPUT);
}

void C65X::start_programming() {
    C65X::control_bus(true);

    rom_write_idle();
    rom_enable_idle();
    rom_output_idle();
    rom_program_active();
    bus_rom_state(OUTPUT);
}

void C65X::end_programming() {
    rom_write_idle();
    rom_enable_idle();
    rom_output_idle();
    rom_program_idle();
    bus_rom_state(INPUT);
    
    C65X::control_bus(false);
}

bool poll_byte(byte data) {
    byte poll = data ^ 0xff;
    int cycle = 0;
    while (data != poll && cycle++ < 100) {
        rom_output_active();
        rom_enable_active();
        delay_cycle();
        delay_cycle();
        delay_cycle();

        poll = get_bus_data();

        rom_output_idle();
        rom_enable_idle();

        delay(1);
    }
    rom_enable_idle();

    Serial.println(cycle);
    if (cycle >= 100) {
        return false;
    }
    return true;
}

bool C65X::program_bytes(word address, byte *data, byte size) {
    if (size > 64 || size ==0) {
        // maximum write size of 64 bytes
        Serial.println("MWrite too big");
        return false;
    }
    if ((address & 0xffc0) != ((address + size - 1) & 0xffc0)) {
        // the 64 byte range must be in the same "segment"
        Serial.println("MStart/End not in same segment");
        return false;
    }
    rom_write_idle();
    rom_enable_idle();
    rom_output_idle();

    bus_address_state_reg(STATE_REG_OUTPUT);
    bus_data_state_reg(STATE_REG_OUTPUT);
    set_bus_address(address);

    delay_cycle();
    rom_enable_active();

    for (byte i = 0; i < size; i++) {
        set_bus_address_low(address + i);
        set_bus_data(data[i]);

        delay_cycle();
        rom_write_active();
        delay_cycle();
        delay_cycle();
        rom_write_idle();
        delay_cycle();
    }

    bus_data_state_reg(STATE_REG_INPUT);
    rom_enable_idle();

    bool result = poll_byte(data[size-1]);

    return result;
}

