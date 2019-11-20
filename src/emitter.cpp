#include "emitter.h"
#include "debug.h"
#include  <iomanip>

Emitter::Emitter(OpcodeTranslator* opcodeTranslator)
{
    mOpcodeTranslator = opcodeTranslator;
    mOutput.resize(0x10000); // TODO
    memset(mOutput.data(), 0xff, 0x10000);
}

uint16_t Emitter::FlipEndianness(uint16_t val)
{
    const uint16_t l = val << 8;
    const uint16_t r = val >> 8;
    return l | r; // swap order of the two bytes
}

void Emitter::SkipBytes(size_t size)
{
    LOG_INFO() << "(uninitialised) x" << size << " bytes";
    mCurrentLocation += size;
}

void Emitter::SetWritePos(size_t pos)
{
    mCurrentLocation = pos;
}

void Emitter::EmitData(const char* data, size_t size)
{
    LOG_INFO() << "(data) x" << size << " bytes";
    memcpy(mOutput.data() + size, data, size);
    mCurrentLocation += size;
}

uint16_t Emitter::Emit(const char* op)
{
    return Emit(op, EAddressingMode::Implied, 0); // TODO: we can simplify this
}

uint16_t Emitter::Emit(const char* op, EAddressingMode addrMode, uint16_t val)
{
    Opcode opcode;
    if (!mOpcodeTranslator->GetOpcode(op, addrMode, opcode))
    {
        LOG_ERROR() << "Failed to emit: " << op << "(" << (int)addrMode << ") " << val;
        return 0;
    }

    // Flip endianness
    const uint8_t val8 = static_cast<uint8_t>(val);
    const uint16_t val16 = val;// FlipEndianness(val);

    // Claculate operand length
    uint16_t operandLen = 0;
    switch (addrMode)
    {
    case EAddressingMode::Absolute:
        operandLen = 2;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(4) << std::hex << val16;
        break;
    case EAddressingMode::AbsoluteX:
        operandLen = 2;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(4) << std::hex << val16;
        break;
    case EAddressingMode::AbsoluteY:
        operandLen = 2;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(4) << std::hex << val16;
        break;
    case EAddressingMode::Accumulator:
        operandLen = 0;
        LOG_INFO() << opcode.mName;
        break;
    case EAddressingMode::Immediate:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " #$" << std::setfill('0') << std::setw(2) << std::hex << val;
        break;
    case EAddressingMode::Implied:
        operandLen = 0;
        LOG_INFO() << opcode.mName;
        break;
    case EAddressingMode::Indirect:
        operandLen = 2;
        LOG_INFO() << opcode.mName << " ($" << std::setfill('0') << std::setw(4) << std::hex << val16 << ")";
        break;
    case EAddressingMode::IndirectX:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " ($" << std::setfill('0') << std::setw(2) << std::hex << val << ",X)";
        break;
    case EAddressingMode::IndirectY:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " ($" << std::setfill('0') << std::setw(2) << std::hex << val << ",Y)";
        break;
    case EAddressingMode::ZeroPage:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(2) << std::hex << val;
        break;
    case EAddressingMode::ZeroPageX:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(2) << std::hex << val << ",X";
        break;
    case EAddressingMode::ZeroPageY:
        operandLen = 1;
        LOG_INFO() << opcode.mName << " $" << std::setfill('0') << std::setw(2) << std::hex << val << ",Y";
        break;
    }

    // Write opcode
    memcpy(mOutput.data() + mCurrentLocation, &opcode.mCode, 1);

    // Wwite operand
    if (operandLen == 1)
    {
        memcpy(mOutput.data() + mCurrentLocation + 1, &val8, 1);
    }
    else if (operandLen == 2)
    {
        memcpy(mOutput.data() + mCurrentLocation + 1, &val16, 2);
    }

    uint16_t bytesWritten = operandLen + 1;
    mCurrentLocation += bytesWritten;

    return bytesWritten;
}
