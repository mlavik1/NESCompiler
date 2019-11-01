#pragma once
#include "compilation_unit.h"
#include "emitter.h"
#include <stdint.h>

class CodeGenerator
{
private:
    CompilationUnit * mCompilationUnit;
    Emitter* emitter;
    uint16_t mPrgLoc;
    uint16_t mCurrLoc;

public:
    CodeGenerator(CompilationUnit* compilationUnit);

    void EmitExpression(Expression* node);
    void EmitStatement(Statement* node);
    void EmitFunction(FunctionDefinition* node);
    void EmitNode(Node* node);
    void Generate();
};
