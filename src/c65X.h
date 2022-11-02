//
// Created by Mark Gastel on 2022-10-29.
//

#ifndef MONITOR_C65X_H
#define MONITOR_C65X_H

class C65X {
public:
    static C65X& get_instance() {
        static C65X instance;
        return instance;
    }

    void initialize();
    void enable_system_clock(bool enable);
    void clock_cycles(int cycles);

    void set_break_point(word address);
    void clear_break_point();

    bool step();
    void bus_enable(bool enable);
    void control_bus(bool enable);
    void read_address(word address);
    void write_address(word address, byte data);
    void start_programming();
    void end_programming();
    bool program_bytes(word address, byte *data, byte size);

private:
    C65X();
    word break_point;
};
#endif //MONITOR_C65X_H
