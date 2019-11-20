#pragma once

#include <string>
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

class OpcodeTranslator
{
private:
    std::map<std::pair<std::string, EAddressingMode>, Opcode> mOpcodeMap;
    std::map<int, Opcode> mBinaryOpcodeMap;

public:
    OpcodeTranslator();

    bool GetOpcode(const std::string& op, const EAddressingMode addrMode, Opcode& outOpcode) const;
    bool GetOpcode(int value, Opcode& outOpcode) const;
};
