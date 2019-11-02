#include "emitter.h"
#include "debug.h"
#include  <iomanip>

#define SET_OPCODE(index,name,addrmode)\
{\
    Opcode op;\
    op.mName = name;\
    op.mAddressingMode = addrmode;\
    op.mCode = index;\
    mOpcodeMap.emplace(std::pair<std::string, EAddressingMode>(name, addrmode), op);\
}

Emitter::Emitter()
{
    RegisterOpcodes();
    mOutput.resize(0x10000);
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
    auto itOp = mOpcodeMap.find(std::pair<std::string, EAddressingMode>(op, addrMode));
    if (itOp == mOpcodeMap.end())
    {
        LOG_ERROR() << "Failed to emit: " << op << "(" << (int)addrMode << ") " << val;
        return 0;
    }

    // Flip endianness
    const uint8_t val8 = static_cast<uint8_t>(val);
    const uint16_t val16 = val;// FlipEndianness(val);

    const Opcode opcode = itOp->second;

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
        LOG_INFO() << opcode.mName << " #" << std::setfill('0') << std::setw(2) << std::hex << val;
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

void Emitter::RegisterOpcodes()
{
    SET_OPCODE(0x00, "BRK", EAddressingMode::Implied);
    SET_OPCODE(0x01, "ORA", EAddressingMode::IndirectX);
    SET_OPCODE(0x05, "ORA", EAddressingMode::ZeroPage);
    SET_OPCODE(0x06, "ASL", EAddressingMode::ZeroPage);
    SET_OPCODE(0x08, "PHP", EAddressingMode::Implied);
    SET_OPCODE(0x09, "ORA", EAddressingMode::Immediate);
    SET_OPCODE(0x0A, "ASL", EAddressingMode::Accumulator);
    SET_OPCODE(0x0D, "ORA", EAddressingMode::Absolute);
    SET_OPCODE(0x0E, "ASL", EAddressingMode::Absolute);

    SET_OPCODE(0x10, "BPL", EAddressingMode::Immediate);
    SET_OPCODE(0x11, "ORA", EAddressingMode::IndirectY);
    SET_OPCODE(0x15, "ORA", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x16, "ASL", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x18, "CLC", EAddressingMode::Implied);
    SET_OPCODE(0x19, "ORA", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x1D, "ORA", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x1E, "ASL", EAddressingMode::AbsoluteX);

    SET_OPCODE(0x20, "JSR", EAddressingMode::Absolute);
    SET_OPCODE(0x21, "AND", EAddressingMode::IndirectX);
    SET_OPCODE(0x24, "BIT", EAddressingMode::ZeroPage);
    SET_OPCODE(0x25, "AND", EAddressingMode::ZeroPage);
    SET_OPCODE(0x26, "ROL", EAddressingMode::ZeroPage);
    SET_OPCODE(0x28, "PLP", EAddressingMode::Implied);
    SET_OPCODE(0x29, "AND", EAddressingMode::Immediate);
    SET_OPCODE(0x2A, "ROL", EAddressingMode::Accumulator);
    SET_OPCODE(0x2C, "BIT", EAddressingMode::Absolute);
    SET_OPCODE(0x2D, "AND", EAddressingMode::Absolute);
    SET_OPCODE(0x2E, "ROL", EAddressingMode::Absolute);

    SET_OPCODE(0x30, "BMI", EAddressingMode::Immediate);
    SET_OPCODE(0x31, "AND", EAddressingMode::IndirectY);
    SET_OPCODE(0x35, "AND", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x36, "ROL", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x38, "SEC", EAddressingMode::Implied);
    SET_OPCODE(0x39, "AND", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x3D, "AND", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x3E, "ROL", EAddressingMode::AbsoluteX);

    SET_OPCODE(0x40, "RTI", EAddressingMode::Implied);
    SET_OPCODE(0x41, "EOR", EAddressingMode::IndirectX);
    SET_OPCODE(0x45, "EOR", EAddressingMode::ZeroPage);
    SET_OPCODE(0x46, "LSR", EAddressingMode::ZeroPage);
    SET_OPCODE(0x48, "PHA", EAddressingMode::Implied);
    SET_OPCODE(0x49, "EOR", EAddressingMode::Immediate);
    SET_OPCODE(0x4A, "LSR", EAddressingMode::Accumulator);
    SET_OPCODE(0x4C, "JMP", EAddressingMode::Absolute);
    SET_OPCODE(0x4D, "EOR", EAddressingMode::Absolute);
    SET_OPCODE(0x4E, "LSR", EAddressingMode::Absolute);

    SET_OPCODE(0x50, "BVC", EAddressingMode::Immediate); // TODO: add cycles if branch is taken
    SET_OPCODE(0x51, "EOR", EAddressingMode::IndirectY);
    SET_OPCODE(0x55, "EOR", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x56, "LSR", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x58, "CLI", EAddressingMode::Implied);
    SET_OPCODE(0x59, "EOR", EAddressingMode::AbsoluteY);
    SET_OPCODE(0x5D, "EOR", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x5E, "LSR", EAddressingMode::AbsoluteX);

    SET_OPCODE(0x60, "RTS", EAddressingMode::Implied);
    SET_OPCODE(0x61, "ADC", EAddressingMode::IndirectX);
    SET_OPCODE(0x65, "ADC", EAddressingMode::ZeroPage);
    SET_OPCODE(0x66, "ROR", EAddressingMode::ZeroPage);
    SET_OPCODE(0x68, "PLA", EAddressingMode::Implied);
    SET_OPCODE(0x69, "ADC", EAddressingMode::Immediate);
    SET_OPCODE(0x6A, "ROR", EAddressingMode::Accumulator);
    SET_OPCODE(0x6C, "JMP", EAddressingMode::Indirect);
    SET_OPCODE(0x6D, "ADC", EAddressingMode::Absolute);
    SET_OPCODE(0x6E, "ROR", EAddressingMode::Absolute);

    SET_OPCODE(0x70, "BVS", EAddressingMode::Immediate);
    SET_OPCODE(0x71, "ADC", EAddressingMode::IndirectY);
    SET_OPCODE(0x75, "ADC", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x76, "ROR", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x78, "SEI", EAddressingMode::Implied);
    SET_OPCODE(0x79, "ADC", EAddressingMode::AbsoluteY);
    SET_OPCODE(0x7D, "ADC", EAddressingMode::AbsoluteX);
    SET_OPCODE(0x7E, "ROR", EAddressingMode::AbsoluteX);

    SET_OPCODE(0x81, "STA", EAddressingMode::IndirectX);
    SET_OPCODE(0x84, "STY", EAddressingMode::ZeroPage);
    SET_OPCODE(0x85, "STA", EAddressingMode::ZeroPage);
    SET_OPCODE(0x86, "STX", EAddressingMode::ZeroPage);
    SET_OPCODE(0x88, "DEY", EAddressingMode::Implied);
    SET_OPCODE(0x8A, "TXA", EAddressingMode::Implied);
    SET_OPCODE(0x8C, "STY", EAddressingMode::Absolute);
    SET_OPCODE(0x8D, "STA", EAddressingMode::Absolute);
    SET_OPCODE(0x8E, "STX", EAddressingMode::Absolute);

    SET_OPCODE(0x90, "BCC", EAddressingMode::Immediate);
    SET_OPCODE(0x91, "STA", EAddressingMode::IndirectY);
    SET_OPCODE(0x94, "STY", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x95, "STA", EAddressingMode::ZeroPageX);
    SET_OPCODE(0x96, "STX", EAddressingMode::ZeroPageY);
    SET_OPCODE(0x98, "TYA", EAddressingMode::Implied);
    SET_OPCODE(0x99, "STA", EAddressingMode::AbsoluteY);
    SET_OPCODE(0x9A, "TXS", EAddressingMode::Implied);
    SET_OPCODE(0x9D, "STA", EAddressingMode::AbsoluteX);

    SET_OPCODE(0xA0, "LDY", EAddressingMode::Immediate);
    SET_OPCODE(0xA1, "LDA", EAddressingMode::IndirectX);
    SET_OPCODE(0xA2, "LDX", EAddressingMode::Immediate);
    SET_OPCODE(0xA4, "LDY", EAddressingMode::ZeroPage);
    SET_OPCODE(0xA5, "LDA", EAddressingMode::ZeroPage);
    SET_OPCODE(0xA6, "LDX", EAddressingMode::ZeroPage);
    SET_OPCODE(0xA8, "TAY", EAddressingMode::Implied);
    SET_OPCODE(0xA9, "LDA", EAddressingMode::Immediate);
    SET_OPCODE(0xAA, "TAX", EAddressingMode::Implied);
    SET_OPCODE(0xAC, "LDY", EAddressingMode::Absolute);
    SET_OPCODE(0xAD, "LDA", EAddressingMode::Absolute);
    SET_OPCODE(0xAE, "LDX", EAddressingMode::Absolute);

    SET_OPCODE(0xB0, "BCS", EAddressingMode::Immediate);
    SET_OPCODE(0xB1, "LDA", EAddressingMode::IndirectY);
    SET_OPCODE(0xB4, "LDY", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xB5, "LDA", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xB6, "LDX", EAddressingMode::ZeroPageY);
    SET_OPCODE(0xB8, "CLV", EAddressingMode::Implied);
    SET_OPCODE(0xB9, "LDA", EAddressingMode::AbsoluteY);
    SET_OPCODE(0xBA, "TSX", EAddressingMode::Implied);
    SET_OPCODE(0xBC, "LDY", EAddressingMode::AbsoluteX);
    SET_OPCODE(0xBD, "LDA", EAddressingMode::AbsoluteX);
    SET_OPCODE(0xBE, "LDX", EAddressingMode::AbsoluteY);

    SET_OPCODE(0xC0, "CPY", EAddressingMode::Immediate);
    SET_OPCODE(0xC1, "CMP", EAddressingMode::IndirectX);
    SET_OPCODE(0xC4, "CPY", EAddressingMode::ZeroPage);
    SET_OPCODE(0xC5, "CMP", EAddressingMode::ZeroPage);
    SET_OPCODE(0xC6, "DEC", EAddressingMode::ZeroPage);
    SET_OPCODE(0xC8, "INY", EAddressingMode::Implied);
    SET_OPCODE(0xC9, "CMP", EAddressingMode::Immediate);
    SET_OPCODE(0xCA, "DEX", EAddressingMode::Implied);
    SET_OPCODE(0xCC, "CPY", EAddressingMode::Absolute);
    SET_OPCODE(0xCD, "CMP", EAddressingMode::Absolute);
    SET_OPCODE(0xCE, "DEC", EAddressingMode::Absolute);

    SET_OPCODE(0xD0, "BNE", EAddressingMode::Immediate);
    SET_OPCODE(0xD1, "CMP", EAddressingMode::IndirectY);
    SET_OPCODE(0xD5, "CMP", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xD6, "DEC", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xD8, "CLD", EAddressingMode::Implied);
    SET_OPCODE(0xD9, "CMP", EAddressingMode::AbsoluteY);
    SET_OPCODE(0xDD, "CMP", EAddressingMode::AbsoluteX);
    SET_OPCODE(0xDE, "DEC", EAddressingMode::AbsoluteX);

    SET_OPCODE(0xE0, "CPX", EAddressingMode::Immediate);
    SET_OPCODE(0xE1, "SBC", EAddressingMode::IndirectX);
    SET_OPCODE(0xE4, "CPX", EAddressingMode::ZeroPage);
    SET_OPCODE(0xE5, "SBC", EAddressingMode::ZeroPage);
    SET_OPCODE(0xE6, "INC", EAddressingMode::ZeroPage);
    SET_OPCODE(0xE8, "INX", EAddressingMode::Implied);
    SET_OPCODE(0xE9, "SBC", EAddressingMode::Immediate);
    SET_OPCODE(0xEA, "NOP", EAddressingMode::Implied);
    SET_OPCODE(0xEC, "CPX", EAddressingMode::Absolute);
    SET_OPCODE(0xED, "SBC", EAddressingMode::Absolute);
    SET_OPCODE(0xEE, "INC", EAddressingMode::Absolute);

    SET_OPCODE(0xF0, "BEQ", EAddressingMode::Immediate);
    SET_OPCODE(0xF1, "SBC", EAddressingMode::IndirectY);
    SET_OPCODE(0xF5, "SBC", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xF6, "INC", EAddressingMode::ZeroPageX);
    SET_OPCODE(0xF8, "SED", EAddressingMode::Implied);
    SET_OPCODE(0xF9, "SBC", EAddressingMode::AbsoluteY);
    SET_OPCODE(0xFD, "SBC", EAddressingMode::AbsoluteX);
    SET_OPCODE(0xFE, "INC", EAddressingMode::AbsoluteX);
}
