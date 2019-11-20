#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <stdint.h>
#include <map>
#include <vector>
#include "opcode.h"

class Emitter
{
private:
    uint16_t mCurrentLocation = 0;
    std::vector<char> mOutput;

    OpcodeTranslator* mOpcodeTranslator;

    uint16_t FlipEndianness(uint16_t val);

public:
    Emitter(OpcodeTranslator* opcodeTranslator);

    void SkipBytes(size_t size);
    void SetWritePos(size_t pos);
    void EmitData(const char* data, size_t size);
    uint16_t Emit(const char* op);
    uint16_t Emit(const char* op, EAddressingMode addrMode, uint16_t val);

    uint16_t GetCurrentLocation() { return mCurrentLocation; }

    char* GetData() { return mOutput.data(); };
    size_t GetDataSize() { return static_cast<size_t>(mCurrentLocation); }
};
