#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <stdint.h>
#include <map>

enum EAddressingMode
{
    Accumulator,
    Immediate,	// uses value directly (no memory lookup)
    ZeroPage,	// first 256 bytes
    ZeroPageX,
    ZeroPageY,
    Absolute,	// full 16 bit address - LSB MSB
    AbsoluteX,
    AbsoluteY,
    Indirect,
    IndirectX,
    IndirectY,
    Implied
};

struct Opcode
{
    std::string mName;
    EAddressingMode mAddressingMode;
    int mCode;
};

class Emitter
{
private:
    std::map<std::pair<std::string, EAddressingMode>, Opcode> mOpcodeMap;

    uint16_t Emit(const char* op, EAddressingMode addrMode, uint8_t val);
    uint16_t Emit(const char* op, EAddressingMode addrMode, uint16_t val);

    void RegisterOpcodes();

public:
    Emitter();
};
