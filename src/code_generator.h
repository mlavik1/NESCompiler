#pragma once
#include "compilation_unit.h"
#include "emitter.h"
#include <stdint.h>
#include <unordered_map>

class CodeGenerator
{
private:
    CompilationUnit * mCompilationUnit;
    Emitter* mEmitter;
    uint16_t mNextVarAddr = 0x0000;
    std::unordered_map<std::string, uint16_t> mFuncRetAddrs; // TODO: remove this hack

    void RegisterBuiltinSymbol(std::string name, uint16_t size);
    void SetIdentifierSymSize(Symbol* sym);

    uint16_t RequestVarAddr(uint16_t bytes);

public:
    CodeGenerator(CompilationUnit* compilationUnit);

    uint16_t EmitExpression(Expression* node);
    void EmitStatement(Statement* node);
    void EmitFunction(FunctionDefinition* node);
    void EmitStruct(StructDefinition* node);
    void EmitNode(Node* node);
    void Generate();
};
