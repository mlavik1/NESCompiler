#pragma once
#include "compilation_unit.h"
#include "emitter.h"
#include <stdint.h>
#include <unordered_map>

enum class EEmitAddrType
{
    None, DataAddress, CodeAddress
};

class EmitAddr
{
public:

    EEmitAddrType mType;
    union
    {
        uint16_t mAddress;
        //uint8_t mValue; // TODO: Allow returning values directly (what about memcpy??? - function params)
    };
    Symbol* mRelativeSymbol = nullptr; // address is relative to this

    EmitAddr() {}

    EmitAddr(EEmitAddrType type, uint16_t addr, Symbol* sym)
    {
        mType = type;
        mAddress = addr;
        mRelativeSymbol = sym;
    }
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

    EmitAddr EmitLiteralExpression(LiteralExpression* litExpr);
    EmitAddr EmitIdentifierExpression(IdentifierExpression* identExpr);
    EmitAddr EmitFuncCallExpression(FunctionCallExpression* callExrp);
    EmitAddr EmitBinOpExpression(BinaryOperationExpression* binOpExpr);
    EmitAddr EmitExpression(Expression* node);
    void EmitControlStatement(ControlStatement* node);
    void EmitStatement(Statement* node);
    void EmitFunction(FunctionDefinition* node);
    void EmitStruct(StructDefinition* node);
    void EmitInlineAssembly(InlineAssemblyStatement* node);
    void EmitBlock(Block* node);
    void EmitNode(Node* node);
    void Generate();
};
