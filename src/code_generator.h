#pragma once
#include "compilation_unit.h"
#include "emitter.h"
#include <stdint.h>
#include <unordered_map>

struct EmitAddr
{
    enum class EAddrType
    {
        None, DataAddress, CodeAddress, DataSymbol, CodeSymbol
    } mType;
    union
    {
        uint16_t mAddress;
        //uint8_t mValue; // TODO: Allow returning values directly (what about memcpy??? - function params)
    };
    Symbol* mRelativeSymbol; // address is relative to this
};

class DataAllocator
{
private:
    uint16_t mNextVarAddr = 0x0000;
public:
    uint16_t RequestVarAddr(uint16_t bytes);
};

class CodeGenerator
{
private:
    CompilationUnit * mCompilationUnit;
    Emitter* mEmitter;
    std::unordered_map<std::string, EmitAddr> mFuncRetAddrs; // TODO: remove this hack
    DataAllocator* mDataAllocator;

    void RegisterBuiltinSymbol(std::string name, uint16_t size);
    void SetIdentifierSymSize(Symbol* sym);

    uint16_t Emit(const std::string& op, const EAddressingMode addrMode, const EmitAddr addr);

public:
    CodeGenerator(CompilationUnit* compilationUnit, Emitter* emitter, DataAllocator* dataAllocator);

    EmitAddr EmitExpression(Expression* node);
    void EmitStatement(Statement* node);
    void EmitFunction(FunctionDefinition* node);
    void EmitStruct(StructDefinition* node);
    void EmitInlineAssembly(InlineAssemblyStatement* node);
    void EmitNode(Node* node);
    void Generate();
};
