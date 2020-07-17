#pragma once
#include "compilation_unit.h"
#include "emitter.h"
#include <stdint.h>
#include <unordered_map>

enum class EProcReg
{
    A, X, Y
};

enum class EAccumulatorArithmeticOp
{
    ADC, SBC, AND
};

enum class EJumpType
{
    JMP, JSR
};

enum class EBranchType
{
    BEQ, BNE, BPL, BMI, BCS
};

enum class EOperandType
{
    None, Value, DataAddress, CodeAddress
};

class EmitOperand
{
public:

    EOperandType mType;
    union
    {
        uint16_t mAddress;
        uint8_t mValue;
        EProcReg mRegister;
    };
    Symbol* mRelativeSymbol = nullptr; // address is relative to this

    EmitOperand() {}

    EmitOperand(EOperandType type, uint16_t addr, Symbol* sym)
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
    std::unordered_map<std::string, EmitOperand> mFuncRetAddrs; // TODO: remove this hack
    DataAllocator* mDataAllocator;

    std::unordered_map<EProcReg, EmitOperand> mRegisterContent;

    void CacheRegisterContent(EProcReg reg, EmitOperand val);
    bool RegisterContains(EProcReg reg, EmitOperand val);
    void ClearRegisterContentCache(EProcReg reg);

    const char* GetLoadOpcode(const EProcReg reg);
    const char* GetStoreOpcode(const EProcReg reg);
    const char* GetCmpOpcode(const EProcReg reg);
    const char* GetAccArithOp(const EAccumulatorArithmeticOp op);
    const char* GetBranchOp(const EBranchType type);

    void RegisterBuiltinSymbol(std::string name, uint16_t size);
    void SetIdentifierSymSize(Symbol* sym);

    void ConvertToAddress(EmitOperand& operand);
    void Emit(const char* op);
    void EmitRelocatedAddress(const std::string& op, const EAddressingMode addrMode, const uint16_t addr);
    void EmitRelocatedSymbol(const std::string& op, const EAddressingMode addrMode, const Symbol* sym, const uint16_t offset = 0);
    void EmitLoad(const EProcReg reg, const EmitOperand operand);
    void EmitStore(const EProcReg reg, const EmitOperand operand);
    void EmitStore(const EmitOperand src, const EmitOperand dst);
    void EmitBranch(EBranchType type, int8_t offset);
    void EmitBranchAt(EBranchType type, uint8_t offset, uint16_t branchCodeAddr);
    void EmitCompare(EProcReg reg, EmitOperand operand1, EmitOperand operand2);
    void EmitCompare(EProcReg reg, EmitOperand operand);
    void EmitAcumulatorArithmetic(EAccumulatorArithmeticOp op, EmitOperand operand);
    void EmitJump(EJumpType type, EmitOperand operand);

public:
    CodeGenerator(CompilationUnit* compilationUnit, Emitter* emitter, DataAllocator* dataAllocator);

    EmitOperand EmitLiteralExpression(LiteralExpression* litExpr);
    EmitOperand EmitIdentifierExpression(IdentifierExpression* identExpr);
    EmitOperand EmitFuncCallExpression(FunctionCallExpression* callExrp);
    EmitOperand EmitBinOpExpression(BinaryOperationExpression* binOpExpr);
    EmitOperand EmitExpression(Expression* node);
    void EmitControlStatement(ControlStatement* node);
	void EmitIfControlStatement(ControlStatement* node);
	void EmitWhileControlStatement(ControlStatement* node);
	void EmitStatement(Statement* node);
    void EmitFunction(FunctionDefinition* node);
    void EmitStruct(StructDefinition* node);
    void EmitInlineAssembly(InlineAssemblyStatement* node);
    void EmitBlock(Block* node);
    void EmitNode(Node* node);
    void Generate();
};
