#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <stdint.h>
#include <map>
#include <vector>

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
    uint16_t mCurrentLocation = 0xC000;
    std::vector<char> mOutput;

    void RegisterOpcodes();
    uint16_t FlipEndianness(uint16_t val);

public:
    Emitter();

    void SkipBytes(size_t size);
    void EmitData(const char* data, size_t size);
    uint16_t Emit(const char* op);
    uint16_t Emit(const char* op, EAddressingMode addrMode, uint16_t val);

    uint16_t GetCurrentLocation() { return mCurrentLocation; }

    char* GetData() { return mOutput.data(); };
};
