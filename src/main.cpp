#include <arduino.h>
#include <string.h>
#include "c65X.h"


static C65X c65 = C65X::get_instance();
bool tracing_enabled = false;
bool step_enabled = false;

void message(char *msg) {
    Serial.print("M");
    Serial.println(msg);
}

void setup() {
    Serial.begin(57600);
    c65.initialize();
    message((char *) "Monitor starting");
}

void start_step() {
    if (!step_enabled) {
        message((char *)"Single step enabled");
        tracing_enabled = false;
        step_enabled = true;
        c65.enable_system_clock(false);
    }
    c65.step();
}

void start_tracing() {
    if (!tracing_enabled) {
        message((char *)"Tracing enabled");
        tracing_enabled = true;
        step_enabled = false;
        c65.enable_system_clock(false);
    }
}

void resume_processing() {
    message((char *)"Resuming normal processing");
    tracing_enabled = false;
    step_enabled = false;
    c65.enable_system_clock(true);
    c65.clear_break_point();
}

void trace() {
    bool  bpf = c65.step();
    if (bpf) {
        message((char *)"Break point found");
        tracing_enabled = false;
        step_enabled = true;
    }
}

byte blocking_serial_read() {
    while (! Serial.available()) {}
    return Serial.read();
}

unsigned long read_hex(int size) {
    char buffer[9];
    int i;
    char *end;

    for (i = 0; i < size; i++) {
        buffer[i] = (char) blocking_serial_read();
    }
    buffer[i] = '\0';
    return strtoul(buffer, &end, 16);
}

void set_break_point() {
    char buffer[32];
    word address = (word) read_hex(4);
    sprintf(buffer, "Setting break point to %04x", address);
    message(buffer);
    start_step();
    c65.set_break_point(address);
}

void read_memory() {
    char buffer[32];

    tracing_enabled = false;
    step_enabled = false;

    word address = (word) read_hex(4);
    byte size = (byte) read_hex(2);

    message((char *) "Taking control of bus");
    c65.control_bus(true);

    sprintf(buffer, "Reading memory at %04x", address);
    message(buffer);

    for (byte i = 0; i < size; i++) {
        c65.read_address(address + (word) i);
    }

    c65.control_bus(false);
    message((char *) "Bus released");
}

void write_memory() {
    char buffer[32];

    tracing_enabled = false;
    step_enabled = false;

    word address = (word) read_hex(4);
    byte data = (byte) read_hex(2);

    message((char *) "Taking control of bus");
    c65.control_bus(true);

    sprintf(buffer, "Setting memory at %04x to %02x", address, data);
    message(buffer);

    c65.write_address(address, data);

    c65.control_bus(false);
    message((char *) "Bus released");
}

void loop() {
    if (Serial.available() > 0) {
        byte incomingByte = Serial.read();
        switch (incomingByte) {
            case ':':
                // start of an Intel HEX file, begin programming EEPROM
//                handleProgramming();
                break;
            case 'b':
                // set a break point on a memory address (does not need to be an instruction)
                set_break_point();
                break;
            case 's':
                // do a step
                start_step();
                break;
            case 't':
                // enter trace mode
                start_tracing();
                break;
            case 'q':
                // resume normal processing
                resume_processing();
                break;
            case 'r':
                // read from ram and return a list
                read_memory();
                break;
            case 'w':
                // write a byte to a ram address
                write_memory();
                break;
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                // do nothing for spaces
                break;
            default:
                // unsupported character
                Serial.print("Unsupported character received: ");
                Serial.println(incomingByte, HEX);
                break;
        }
    }

    // Serial will be true if Serial Monitor is opened on the computer
    // This only works on Teensy and Arduino boards with native USB (Leonardo, Micro...), and indicates whether or not the USB CDC serial connection is open
    // On boards with separate USB interface, this always return true (eg. Uno, Nano, Mini, Mega)
    if (Serial) {
        if (tracing_enabled) {
            trace();
        }
    } else {
        if (tracing_enabled||step_enabled) {
            resume_processing();
        }
    }
}