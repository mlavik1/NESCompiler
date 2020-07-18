#include "code_generator.h"
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <exception>

uint16_t DataAllocator::RequestVarAddr(uint16_t bytes)
{
    // Avoid collision with the stack
    if (mNextVarAddr + bytes >= 0x0100)
        mNextVarAddr = 0x0200;

    const uint16_t addr = mNextVarAddr;
    mNextVarAddr += bytes;

    assert(mNextVarAddr <= 0x0800); // End of 2KB internal RAM

    return addr;
}

void CodeGenerator::CacheRegisterContent(EProcReg reg, EmitOperand val)
{
    // TODO: Allow caching more than one operand.
    // example case:
    // LDA #1
    // STA $0000
    // STA $0001 => #1 == $0000 == $0001
    mRegisterContent[reg] = val;
}

bool CodeGenerator::RegisterContains(EProcReg reg, EmitOperand val)
{
    return (memcmp(&mRegisterContent[reg], &val, sizeof(EmitOperand)) == 0);
}

void CodeGenerator::ClearRegisterContentCache(EProcReg reg)
{
    mRegisterContent[reg] = EmitOperand();
}

const char* CodeGenerator::GetLoadOpcode(const EProcReg reg)
{
    switch (reg)
    {
    case EProcReg::A:
            return "LDA";
    case EProcReg::X:
        return "LDX";
    case EProcReg::Y:
        return "LDY";
    }
}

const char* CodeGenerator::GetStoreOpcode(const EProcReg reg)
{
    switch (reg)
    {
    case EProcReg::A:
        return "STA";
    case EProcReg::X:
        return "STX";
    case EProcReg::Y:
        return "STY";
    }
}

const char* CodeGenerator::GetCmpOpcode(const EProcReg reg)
{
    switch (reg)
    {
    case EProcReg::A:
        return "CMP";
    case EProcReg::X:
        return "CPX";
    case EProcReg::Y:
        return "CPY";
    }
}

const char* CodeGenerator::GetAccArithOp(const EAccumulatorArithmeticOp op)
{
    switch (op)
    {
    case EAccumulatorArithmeticOp::ADC:
        return "ADC";
    case EAccumulatorArithmeticOp::SBC:
        return "SBC";
    case EAccumulatorArithmeticOp::AND:
        return "AND";
    }
}

const char* CodeGenerator::GetBranchOp(const EBranchType type)
{
    switch (type)
    {
    case EBranchType::BCS:
        return "BCS";
    case EBranchType::BEQ:
        return "BEQ";
    case EBranchType::BMI:
        return "BMI";
    case EBranchType::BNE:
        return "BNE";
    case EBranchType::BPL:
        return "BPL";
    }
}

CodeGenerator::CodeGenerator(CompilationUnit* compilationUnit, Emitter* emitter, DataAllocator* dataAllocator)
{
    mCompilationUnit = compilationUnit;
    mEmitter = emitter;
    mDataAllocator = dataAllocator;

    mRegisterContent[EProcReg::A] = EmitOperand();
    mRegisterContent[EProcReg::X] = EmitOperand();
    mRegisterContent[EProcReg::Y] = EmitOperand();

    // Register built-in types
    RegisterBuiltinSymbol("uint8_t", 1);
}

void CodeGenerator::RegisterBuiltinSymbol(std::string name, uint16_t size)
{
    Symbol* sym = new Symbol();
    sym->mName = sym->mUniqueName = name;
    sym->mSize = size;
    sym->mSymbolType = ESymbolType::BuiltInType;
    mCompilationUnit->mSymbolTable[name] = sym;
}

void CodeGenerator::ConvertToAddress(EmitOperand& operand)
{
    if (operand.mType == EOperandType::Value)
    {
        EmitOperand src = operand;
        operand.mType = EOperandType::DataAddress;
        operand.mAddress = mDataAllocator->RequestVarAddr(1); // TODO: support more than 1 byte literals
        EmitStore(src, operand);
    }
}

void CodeGenerator::Emit(const char* op)
{
    mEmitter->Emit(op);
}

void CodeGenerator::EmitRelocatedAddress(const std::string& op, const EAddressingMode addrMode, const uint16_t operand)
{
    mEmitter->Emit(op.c_str(), addrMode, operand);
    mCompilationUnit->mRelocationText.mRelativeAddresses.push_back(mEmitter->GetCurrentLocation() - 2);
}

void CodeGenerator::EmitRelocatedSymbol(const std::string& op, const EAddressingMode addrMode, const Symbol* sym, const uint16_t offset)
{
    mEmitter->Emit(op.c_str(), addrMode, sym->mAddress + offset);
    mCompilationUnit->mRelocationText.mSymAddrRefs.push_back({ mEmitter->GetCurrentLocation() - 2, sym->mUniqueName });
}


void CodeGenerator::EmitLoad(const EProcReg reg, const EmitOperand operand)
{
    // TODO: We need to be absolutely sure that the address has not been written to since last load
    if (RegisterContains(reg, operand))
        return;

    const char* op = GetLoadOpcode(reg);

    switch (operand.mType)
    {
    case EOperandType::None:
        printf("ERROR: EmitLoad called with None address.\n");
        return;
    case EOperandType::Value:
        mEmitter->Emit(op, EAddressingMode::Immediate, operand.mValue);
        break;
    case EOperandType::DataAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            mEmitter->Emit(op, EAddressingMode::Absolute, operand.mAddress);
        break;
    case EOperandType::CodeAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            EmitRelocatedAddress(op, EAddressingMode::Absolute, operand.mAddress);
        break;
    }

    CacheRegisterContent(reg, operand);
}

void CodeGenerator::EmitStore(const EProcReg reg, const EmitOperand operand)
{
    const char* op = GetStoreOpcode(reg);

    switch (operand.mType)
    {
    case EOperandType::None:
        printf("ERROR: EmitLoad called with None address. STA/STX/STY must be called with memory address.\n");
        return;
    case EOperandType::Value:
        printf("ERROR: EmitStore called with Value operand. STA/STX/STY must be called with memory address.\n");
        return;
    case EOperandType::DataAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            mEmitter->Emit(op, EAddressingMode::Absolute, operand.mAddress);
        break;
    case EOperandType::CodeAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            EmitRelocatedAddress(op, EAddressingMode::Absolute, operand.mAddress);
        break;
    }

    if (RegisterContains(reg, operand))
        ClearRegisterContentCache(reg);
}

void CodeGenerator::EmitStore(const EmitOperand src, const EmitOperand dst)
{
    EmitLoad(EProcReg::A, src);
    EmitStore(EProcReg::A, dst);
}

void CodeGenerator::EmitBranch(EBranchType type, int8_t offset)
{
    const char* op = GetBranchOp(type);

    const uint8_t absOffset = static_cast<uint8_t>(abs(offset));
    if (absOffset > 127)
    {
        printf("ERROR: Control statement body too large. Max displacement exceeded.\n");
        return;
    }

    const uint8_t displacement = (offset >= 0) ? (absOffset) : (absOffset | 0b10000000); // absolute offset, with first bit as sign bit
    mEmitter->Emit(op, EAddressingMode::Immediate, displacement);
}

void CodeGenerator::EmitBranchAt(EBranchType type, uint8_t offset, uint16_t branchCodeAddr)
{
    uint16_t emitterLoc = mEmitter->GetCurrentLocation();
    mEmitter->SetWritePos(branchCodeAddr);
    EmitBranch(type, offset);
    mEmitter->SetWritePos(emitterLoc);
}

void CodeGenerator::EmitCompare(EProcReg reg, EmitOperand operand1, EmitOperand operand2)
{
    // If content of second operand is already in register, swap the parameters
    if (memcmp(&mRegisterContent[reg], &operand2, sizeof(EmitOperand)) == 0)
    {
        EmitCompare(reg, operand2, operand1);
        return;
    }

    EmitLoad(reg, operand1);

    EmitCompare(reg, operand2);
}

void CodeGenerator::EmitCompare(EProcReg reg, EmitOperand operand)
{
    const char* op = GetCmpOpcode(reg);

    switch (operand.mType)
    {
    case EOperandType::None:
        printf("ERROR: EmitCompare called with None address. It must be called with memory address or immediate value.\n");
        assert(0);
        return;
    case EOperandType::Value:
        mEmitter->Emit(op, EAddressingMode::Immediate, operand.mValue);
        return;
    case EOperandType::DataAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            mEmitter->Emit(op, EAddressingMode::Absolute, operand.mAddress);
        break;
    case EOperandType::CodeAddress:
        printf("ERROR: EmitCompare called with Code address. Why would you do that?\n");
        assert(0);
        break;
    }
}

void CodeGenerator::EmitAcumulatorArithmetic(EAccumulatorArithmeticOp op, EmitOperand operand)
{
    const char* opString = GetAccArithOp(op);

    switch (operand.mType)
    {
    case EOperandType::None:
        printf("ERROR: EmitAcumulatorArithmetic called with None address.\n");
        assert(0);
        return;
    case EOperandType::Value:
        mEmitter->Emit(opString, EAddressingMode::Immediate, operand.mValue);
        return;
    case EOperandType::DataAddress:
        if (operand.mRelativeSymbol != nullptr)
            EmitRelocatedSymbol(opString, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
        else
            mEmitter->Emit(opString, EAddressingMode::Absolute, operand.mAddress);
        break;
    case EOperandType::CodeAddress:
        printf("ERROR: EmitAcumulatorArithmetic called with Code address. Why would you do that?\n");
        assert(0);
        break;
    }

    // Clear register content
    ClearRegisterContentCache(EProcReg::A);
}

void CodeGenerator::EmitJump(EJumpType type, EmitOperand operand)
{
    assert(operand.mType == EOperandType::CodeAddress);

    const char* op = type == EJumpType::JMP ? "JMP" : "JSR";


    if (operand.mRelativeSymbol != nullptr)
        EmitRelocatedSymbol(op, EAddressingMode::Absolute, operand.mRelativeSymbol, operand.mAddress);
    else
        EmitRelocatedAddress(op, EAddressingMode::Absolute, operand.mAddress);
}

void CodeGenerator::SetIdentifierSymSize(Symbol* sym)
{
    ESymbolType type = sym->mSymbolType;
    switch (type)
    {
    case ESymbolType::Variable:
    case ESymbolType::FuncParam:
    {
        Symbol* typeSym = mCompilationUnit->mSymbolTable[sym->mTypeName];
        sym->mSize = typeSym->mSize;
        break;
    }
    default:
        assert(false);
    }
}

EmitOperand CodeGenerator::EmitLiteralExpression(LiteralExpression* litExpr)
{
    if (litExpr->mToken.mTokenType == ETokenType::IntegerLiteral)
    {
        assert(litExpr->mValueType == "uint8_t");
        uint8_t val = static_cast<uint8_t>(litExpr->mToken.mIntValue);
        EmitOperand emitRes;
        emitRes.mType = EOperandType::Value;
        emitRes.mValue = val;
        return emitRes;
    }
    else
        assert(0); // TODO: implement other types
}

EmitOperand CodeGenerator::EmitIdentifierExpression(IdentifierExpression* identExpr)
{
    Symbol* identSym = mCompilationUnit->mSymbolTable[identExpr->mIdentifier];
    return EmitOperand(EOperandType::DataAddress, 0, identSym);
}

EmitOperand CodeGenerator::EmitFuncCallExpression(FunctionCallExpression* callExrp)
{
    Symbol* funcSym = mCompilationUnit->mSymbolTable[callExrp->mFunction];

    // Set parameters
    Symbol* paramSym = funcSym->mChildren ? funcSym->mChildren->mTail : nullptr;
    Expression* paramExpr = callExrp->mParameters;
    while (paramExpr != nullptr)
    {
        // Parameter value expression
        EmitOperand paramExprAddr = EmitExpression(paramExpr);

        if (paramSym->mSize == 1)
            EmitStore(paramExprAddr, EmitOperand(EOperandType::DataAddress, 0, paramSym));
        else
        {
            // Ensure that the expression is stored in memory
            ConvertToAddress(paramExprAddr);

            // Copy byte by byte
            uint16_t currOffset = 0;
            while (currOffset < paramSym->mSize)
            {
                // Load value from expression into A register
                EmitOperand exprChunkAddr = paramExprAddr;
                exprChunkAddr.mAddress += currOffset;
                EmitLoad(EProcReg::A, exprChunkAddr);

                // Store value of A in parameter symbol address
                EmitOperand currParamAddr(EOperandType::DataAddress, currOffset, paramSym); // relative to sym addr (relocated later)
                EmitStore(EProcReg::A, currParamAddr);
                currOffset++;
            }
        }

        paramExpr = static_cast<Expression*>(paramExpr->mNext);
        paramSym = paramSym->mNext;
    }

    // Jump
    EmitOperand jmpAddr(EOperandType::CodeAddress, funcSym->mAddress, funcSym);
    EmitJump(EJumpType::JSR, jmpAddr);

    // Return value
    if (funcSym->mTypeName != "void")
    {
        EmitOperand funcRetAddr = mFuncRetAddrs[funcSym->mUniqueName];

        return funcRetAddr;
    }
    else
    {
        EmitOperand voidAddr;
        voidAddr.mType = EOperandType::None;
        return voidAddr;
    }
}

EmitOperand CodeGenerator::EmitBinOpExpression(BinaryOperationExpression* binOpExpr)
{
    Symbol* valSym = mCompilationUnit->mSymbolTable[binOpExpr->mValueType];

    EmitOperand retAddr;
    retAddr.mType = EOperandType::DataAddress;
    retAddr.mAddress = mDataAllocator->RequestVarAddr(valSym->mSize);

    EmitOperand leftExprAddr = EmitExpression(binOpExpr->mLeftOperand);
    EmitOperand rightExprAddr = EmitExpression(binOpExpr->mRightOperand);

    if (binOpExpr->mValueType == "uint8_t")
    {
        if (binOpExpr->mOperator == "+" || binOpExpr->mOperator == "-")
        {
            EmitLoad(EProcReg::A, leftExprAddr);
            if(binOpExpr->mOperator == "+")
                EmitAcumulatorArithmetic(EAccumulatorArithmeticOp::ADC, rightExprAddr);
            else
                EmitAcumulatorArithmetic(EAccumulatorArithmeticOp::SBC, rightExprAddr);
            EmitStore(EProcReg::A, retAddr);
        }
        else if (binOpExpr->mOperator == "==" || binOpExpr->mOperator == "!=")
        {
            EmitCompare(EProcReg::A, leftExprAddr, rightExprAddr);
            // Branch
            uint16_t branchAddr = mEmitter->GetCurrentLocation();
            if (binOpExpr->mOperator == "==")
                EmitBranch(EBranchType::BEQ, 0); // dummy address (0) is relocated below
            else if(binOpExpr->mOperator == "!=")
                EmitBranch(EBranchType::BNE, 0); // dummy address (0) is relocated below
            // TODO: ">"  "<" (BMI)  ">=" (BPL)  "<="

            // False case
            EmitLoad(EProcReg::A, EmitOperand(EOperandType::Value, 0, nullptr));
            uint16_t jmpAddr = mEmitter->GetCurrentLocation();
            EmitJump(EJumpType::JMP, EmitOperand(EOperandType::CodeAddress, 0, nullptr)); // dummy address (0) is relocated below
            // True case
            uint16_t branchDest = mEmitter->GetCurrentLocation();
            EmitLoad(EProcReg::A, EmitOperand(EOperandType::Value, 1, nullptr));
            uint16_t jmpDest = mEmitter->GetCurrentLocation();

            // Relocate branch/jump destination addresses
            // TODO: Use EmitBranchAt(...) and EmitJumpAt(...)
            const uint16_t branchNextLoc = branchAddr + 2; // instruction after branch
            const uint8_t displacement = (branchDest >= branchNextLoc) ? (branchDest - branchNextLoc) : (branchNextLoc - branchDest) | 0b10000000;
            mEmitter->EmitDataAtPos(branchAddr + 1, reinterpret_cast<const char*>(&displacement), sizeof(displacement));
            mEmitter->EmitDataAtPos(jmpAddr + 1, reinterpret_cast<const char*>(&jmpDest), sizeof(jmpDest));

            // Write result
            EmitStore(EProcReg::A, EmitOperand(EOperandType::DataAddress, retAddr.mAddress, nullptr));
        }
		else if (binOpExpr->mOperator == "=")
		{
			EmitStore(rightExprAddr, leftExprAddr);
		}
    }
    else
        assert(0); // TODO: Implement other types

    return retAddr;
}

EmitOperand CodeGenerator::EmitExpression(Expression* node)
{
    EExpressionType type = node->GetExpressionType();
    switch (type)
    {
    case EExpressionType::Literal:
    {
        return EmitLiteralExpression(static_cast<LiteralExpression*>(node));
        break;
    }
    case EExpressionType::Identifier:
    {
        return EmitIdentifierExpression(static_cast<IdentifierExpression*>(node));
        break;
    }
    case EExpressionType::FunctionCall:
    {
        return EmitFuncCallExpression(static_cast<FunctionCallExpression*>(node));
        break;
    }
    case EExpressionType::UnaryOperation:
    {
        // TODO
        break;
    }
    case EExpressionType::BinaryOperation:
    {
        return EmitBinOpExpression(static_cast<BinaryOperationExpression*>(node));

        break;
    }
    default:
        assert(false);
    }
}

void CodeGenerator::EmitControlStatement(ControlStatement* node)
{
	if (node->mControlStatementType == ControlStatement::EControlStatementType::If)
	{
		EmitIfControlStatement(node);
	}
	else if (node->mControlStatementType == ControlStatement::EControlStatementType::While)
	{
		EmitWhileControlStatement(node);
	}
	else
	{
        throw std::invalid_argument("Control statement not implemented");
	}
}

void CodeGenerator::EmitIfControlStatement(ControlStatement* node)
{
	uint16_t codeAddrStart = mEmitter->GetCurrentLocation();

	// Emit condition expression
	EmitOperand exprAddr = EmitExpression(node->mExpression);

	EmitLoad(EProcReg::A, exprAddr);

	uint16_t condBranchLoc = mEmitter->GetCurrentLocation();

	EmitBranch(EBranchType::BEQ, 0); // relocated below

	// Emit body content
	EmitNode(node->mBody);

	// Jump to end, after executing body
	uint16_t jmpLoc = mEmitter->GetCurrentLocation();
	EmitJump(EJumpType::JMP, EmitOperand(EOperandType::CodeAddress, 0, nullptr)); // relocated below

	uint16_t branchDest = mEmitter->GetCurrentLocation();

	// TODO: Use JMP if displacement is exceeded
	if (std::abs(branchDest - condBranchLoc) > 127)
		printf("ERROR: Control statement body too large. Max displacement exceeded.\n");

	// Relocate branch destination address
	const uint16_t branchNextLoc = condBranchLoc + 2;
	const uint8_t branchDisplacement = static_cast<uint8_t>(branchDest - branchNextLoc);
	mEmitter->EmitDataAtPos(condBranchLoc + 1, reinterpret_cast<const char*>(&branchDisplacement), sizeof(uint8_t));

	// else { ... }
	if (node->mConnectedStatement != nullptr)
	{
		EmitNode(node->mConnectedStatement);
	}

	// end (jump here after executing main body)
	uint16_t endPos = mEmitter->GetCurrentLocation();
	mEmitter->EmitDataAtPos(jmpLoc + 1, reinterpret_cast<const char*>(&endPos), sizeof(uint16_t));
}

void CodeGenerator::EmitWhileControlStatement(ControlStatement* node)
{
	uint16_t codeAddrStart = mEmitter->GetCurrentLocation();

	// Emit condition expression
	EmitOperand exprAddr = EmitExpression(node->mExpression);

	EmitLoad(EProcReg::A, exprAddr);

	uint16_t condBranchLoc = mEmitter->GetCurrentLocation();

	EmitBranch(EBranchType::BEQ, 0); // relocated below

	// Emit body content
	EmitNode(node->mBody);

	// jump back to start (after body)
	EmitJump(EJumpType::JMP, EmitOperand(EOperandType::CodeAddress, codeAddrStart, nullptr));

	uint16_t branchDest = mEmitter->GetCurrentLocation();

	// TODO: Use JMP if displacement is exceeded
	if (std::abs(branchDest - condBranchLoc) > 127)
		printf("ERROR: Control statement body too large. Max displacement exceeded.\n");

	// Relocate branch destination address
	const uint16_t branchNextLoc = condBranchLoc + 2;
	const uint8_t branchDisplacement = static_cast<uint8_t>(branchDest - branchNextLoc);
	mEmitter->EmitDataAtPos(condBranchLoc + 1, reinterpret_cast<const char*>(&branchDisplacement), sizeof(uint8_t));
}

void CodeGenerator::EmitStatement(Statement* node)
{
    EStatementType type = node->GetStatementType();
    switch (type)
    {
    case EStatementType::VariableDefinition:
    {
        VarDefStatement* varDefStm = static_cast<VarDefStatement*>(node);
        Symbol* stmsym = mCompilationUnit->mSymbolTable[varDefStm->mName];
        Symbol* typesym = mCompilationUnit->mSymbolTable[varDefStm->mType];
        
        if (stmsym->mAddrType == ESymAddrType::None) // not yet defined
        {
            stmsym->mAddrType = ESymAddrType::Absolute;
            stmsym->mAddress = mDataAllocator->RequestVarAddr(typesym->mSize);
            stmsym->mSize = typesym->mSize; // ??
        }

        if (varDefStm->mExpression != nullptr)
        {
            assert(varDefStm->mExpression->mValueType == varDefStm->mType);

            EmitOperand exprAddr = EmitExpression(varDefStm->mExpression);

            if (typesym->mSize == 1)
                EmitStore(exprAddr, EmitOperand(EOperandType::DataAddress, 0, stmsym));
            else
            {
                // Ensure that the expression is stored in memory
                ConvertToAddress(exprAddr);

                // Copy byte by byte
                uint16_t currOffset = 0;
                while (currOffset < typesym->mSize)
                {
                    // Load value from expression into A register
                    EmitOperand exprChunkAddr = exprAddr;
                    exprChunkAddr.mAddress += currOffset;
                    EmitLoad(EProcReg::A, exprChunkAddr);

                    // Store value of A in parameter symbol address
                    EmitOperand varChunkAddr(EOperandType::DataAddress, currOffset, stmsym);
                    EmitStore(EProcReg::A, varChunkAddr);
                    currOffset++;
                }
            }
        }

        break;
    }
    case EStatementType::ControlStatement:
    {
        EmitControlStatement(static_cast<ControlStatement*>(node));
        break;
    }
    case EStatementType::ReturnStatement:
    {
        ReturnStatement* retStm = static_cast<ReturnStatement*>(node);

        if (retStm->mExpression != nullptr)
        {
            EmitOperand retExprAddr = EmitExpression(retStm->mExpression);

            Symbol* funcSym = mCompilationUnit->mSymbolTable[retStm->mFunction];
            mFuncRetAddrs[funcSym->mUniqueName] = retExprAddr;
        }

        Emit("RTS");

        break;
    }
    case EStatementType::Expression:
    {
        ExpressionStatement* exprStm = static_cast<ExpressionStatement*>(node);
        EmitExpression(exprStm->mExpression);
        break;
    }
    default:
        assert(0); // TODO
    }
}

void CodeGenerator::EmitFunction(FunctionDefinition* node)
{
    // Declaration only?
    if (node->mContent == nullptr)
        return;

    Symbol* funcSym = mCompilationUnit->mSymbolTable[node->mName];
    funcSym->mAddrType = ESymAddrType::Absolute;
    funcSym->mAddress = mEmitter->GetCurrentLocation();

    // Parameters
    VarDefStatement* currParam = static_cast<VarDefStatement*>(node->mParams);
    while (currParam != nullptr)
    {
        // Update symbol
        Symbol* paramSym = mCompilationUnit->mSymbolTable[currParam->mName];
        SetIdentifierSymSize(paramSym);
        // Set address
        paramSym->mAddrType = ESymAddrType::Absolute;
        paramSym->mAddress = mDataAllocator->RequestVarAddr(paramSym->mSize);

        currParam = static_cast<VarDefStatement*>(currParam->mNext);
    }
    
    // Function body
    Node* currContent = node->mContent;
    while (currContent != nullptr)
    {
        EmitNode(currContent);
        currContent = currContent->mNext;
    }

    // void return
    if (node->mType == "void")
        Emit("RTS");

    funcSym->mSize = mEmitter->GetCurrentLocation() - funcSym->mAddress;
}

void CodeGenerator::EmitStruct(StructDefinition* node)
{
    // Declaration only?
    if (node->mContent == nullptr)
        return;

    Symbol* structSym = mCompilationUnit->mSymbolTable[node->mName];
    structSym->mAddrType = ESymAddrType::Absolute;
    structSym->mAddress = mEmitter->GetCurrentLocation();

    Node* currContent = node->mContent;
    while (currContent != nullptr)
    {
        EmitNode(currContent);
        currContent = currContent->mNext;
    }

    structSym->mAddress = mEmitter->GetCurrentLocation() - structSym->mAddress;
}

void CodeGenerator::EmitInlineAssembly(InlineAssemblyStatement* node)
{
    std::transform(node->mOpcodeName.begin(), node->mOpcodeName.end(), node->mOpcodeName.begin(), ::toupper);

    if (node->mOpcodeName == "")
        mEmitter->Emit(node->mOp1.c_str());
    else
    {
        EAddressingMode addrMode = static_cast<EAddressingMode>(-1);

        auto opSymIter = mCompilationUnit->mSymbolTable.find(node->mOp1);

        const bool isSym = opSymIter != mCompilationUnit->mSymbolTable.end();
        const bool isVal = node->mOp1[0] == '#';
        const bool isHex = node->mOp1[isVal ? 1 : 0] == '$';
		const bool isAccum = node->mOp1 == "A";
        const std::string opValStr = node->mOp1.substr((isVal ? 1 : 0) + (isHex ? 1 : 0));

        // Get addressing mode
		if (node->mOp1 == "")
		{
			addrMode = EAddressingMode::Implied;
		}
        else if (isVal)
        {
            addrMode = EAddressingMode::Immediate;
        }
        else if (isSym)
        {
            addrMode = EAddressingMode::Absolute;
        }
		else if (isAccum)
		{
			addrMode = EAddressingMode::Accumulator;
		}
        else
        {
            assert(isHex);
            const size_t opSize = opValStr.size() / 2;
            assert(0 < opSize < 3);

            const bool isAbsolute = opSize == 2;

            if (node->mOp2 == "")
            {
                if (isAbsolute)
                    addrMode = EAddressingMode::Absolute;
                else
                    addrMode = EAddressingMode::ZeroPage;
            }
            else
            {
                std::transform(node->mOp2.begin(), node->mOp2.end(), node->mOp2.begin(), ::tolower);

                if (isAbsolute && node->mOp2 == "x")
                    addrMode = EAddressingMode::AbsoluteX;
                else if (isAbsolute && node->mOp2 == "y")
                    addrMode = EAddressingMode::AbsoluteY;
                else if (node->mOp2 == "x")
                    addrMode = EAddressingMode::ZeroPageX;
                else if (node->mOp2 == "y")
                    addrMode = EAddressingMode::ZeroPageY;
            }
        }
        assert(addrMode != -1);
        
		// Get operand value
		unsigned int opVal = 0;
		if (isSym)
		{
			opVal = static_cast<unsigned int>(opSymIter->second->mAddress);
		}
		else if (isHex)
		{
			std::stringstream ss;
			ss << std::hex << opValStr;
			ss >> opVal;
		}
		else if(addrMode != EAddressingMode::Implied && addrMode != EAddressingMode::Accumulator)
			opVal = std::stoi(opValStr);
		
		mEmitter->Emit(node->mOpcodeName.c_str(), addrMode, static_cast<uint16_t>(opVal));
    }
}

void CodeGenerator::EmitBlock(Block* node)
{
    Node* currNode = node->mNode;
    while (currNode != nullptr)
    {
        EmitNode(currNode);
        currNode = currNode->mNext;
    }
}

void CodeGenerator::EmitNode(Node* node)
{
    ENodeType nodeType = node->GetNodeType();

    switch (nodeType)
    {
    case ENodeType::FunctionDefinition:
        EmitFunction(static_cast<FunctionDefinition*>(node));
        break;
    case ENodeType::Statement:
        EmitStatement(static_cast<Statement*>(node));
        break;
    case ENodeType::StructDefinition:
        EmitStruct(static_cast<StructDefinition*>(node));
        break;
    case ENodeType::InlineAssembly:
        EmitInlineAssembly(static_cast<InlineAssemblyStatement*>(node));
        break;
    case ENodeType::Block:
        EmitBlock(static_cast<Block*>(node));
        break;
    }
}

void CodeGenerator::Generate()
{
    Node* currNode = mCompilationUnit->mRootNode;
    while (currNode != nullptr)
    {
        EmitNode(currNode);
        currNode = currNode->mNext;
    }
}
