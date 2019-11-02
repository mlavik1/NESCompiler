#include "code_generator.h"
#include <assert.h>
#include <fstream>

void CodeGenerator::RegisterBuiltinSymbol(std::string name, uint16_t size)
{
    Symbol* sym = new Symbol();
    sym->mName = sym->mUniqueName = name;
    sym->mSize = size;
    sym->mSymbolType = ESymbolType::BuiltInType;
    mCompilationUnit->mSymbolTable[name] = sym;
}

CodeGenerator::CodeGenerator(CompilationUnit* compilationUnit)
{
    mCompilationUnit = compilationUnit;
    mEmitter = new Emitter();
    
    // Register built-in types
    RegisterBuiltinSymbol("uint8_t", 1);
}

uint16_t CodeGenerator::RequestVarAddr(uint16_t bytes)
{
    // Avoid collision with the stack
    if (mNextVarAddr + bytes >= 0x0100)
        mNextVarAddr = 0x0200;

    const uint16_t addr = mNextVarAddr;
    mNextVarAddr += bytes;

    assert(mNextVarAddr <= 0x0800); // End of 2KB internal RAM

    return addr;
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

uint16_t CodeGenerator::EmitExpression(Expression* node)
{
    EExpressionType type = node->GetExpressionType();
    switch (type)
    {
    case EExpressionType::Literal:
    {
        LiteralExpression* litExpr = static_cast<LiteralExpression*>(node);
        if (litExpr->mToken.mTokenType == ETokenType::IntegerLiteral)
        {
            assert(node->mValueType == "uint8_t");
            uint16_t addr = RequestVarAddr(1);
            uint8_t val = static_cast<uint8_t>(litExpr->mToken.mIntValue);
            mEmitter->Emit("LDA", EAddressingMode::Immediate, val);
            mEmitter->Emit("STA", EAddressingMode::Absolute, addr);
            return addr;
        }
        else
            assert(0); // TODO: implement other types
        break;
    }
    case EExpressionType::Identifier:
    {
        IdentifierExpression* identExpr = static_cast<IdentifierExpression*>(node);
        Symbol* identSym = mCompilationUnit->mSymbolTable[identExpr->mIdentifier];
        return identSym->mAddress;

        break;
    }
    case EExpressionType::FunctionCall:
    {
        FunctionCallExpression* callExrp = static_cast<FunctionCallExpression*>(node);
        Symbol* funcSym = mCompilationUnit->mSymbolTable[callExrp->mFunction];
        
        // Set parameters
        Symbol* paramSym = funcSym->mChildren ? funcSym->mChildren->mTail : nullptr;
        Expression* paramExpr = callExrp->mParameters;
        while (paramExpr != nullptr)
        {
            uint16_t paramExprAddr = EmitExpression(paramExpr);

            uint16_t currOffset = 0;
            while (currOffset < paramSym->mSize)
            {
                mEmitter->Emit("LDA", EAddressingMode::Absolute, paramExprAddr + currOffset);
                mEmitter->Emit("STA", EAddressingMode::Absolute, paramSym->mAddress + currOffset);
                currOffset++;
            }

            paramExpr = static_cast<Expression*>(paramExpr->mNext);
            paramSym = paramSym->mNext;
        }

        // Jump
        mEmitter->Emit("JSR", EAddressingMode::Absolute, funcSym->mAddress);

        // Return value
        if (funcSym->mTypeName != "void")
        {
            uint16_t funcRetAddr = mFuncRetAddrs[funcSym->mUniqueName];

            return funcRetAddr;
        }
        else
            return 0;

        break;
    }
    case EExpressionType::UnaryOperation:
    {
        // TODO
        break;
    }
    case EExpressionType::BinaryOperation:
    {
        BinaryOperationExpression* binOpExpr = static_cast<BinaryOperationExpression*>(node);
        Symbol* valSym = mCompilationUnit->mSymbolTable[binOpExpr->mValueType];

        uint16_t retAddr = RequestVarAddr(valSym->mSize);

        uint16_t leftExprAddr = EmitExpression(binOpExpr->mLeftOperand);
        uint16_t rightExprAddr = EmitExpression(binOpExpr->mRightOperand);

        if (binOpExpr->mValueType == "uint8_t")
        {
            mEmitter->Emit("LDA", EAddressingMode::Absolute, leftExprAddr);
            mEmitter->Emit("ADC", EAddressingMode::Absolute, rightExprAddr);
            mEmitter->Emit("STA", EAddressingMode::Absolute, retAddr);
        }
        else
            assert(0); // TODO: Implement other types

        return retAddr;

        break;
    }
    default:
        assert(false);
    }
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
            stmsym->mAddress = RequestVarAddr(typesym->mSize);
            stmsym->mSize = typesym->mSize; // ??
        }

        if (varDefStm->mExpression != nullptr)
        {
            assert(varDefStm->mExpression->mValueType == varDefStm->mType);

            uint16_t exprAddr = EmitExpression(varDefStm->mExpression);

            uint16_t currOffset = 0;
            while (currOffset < typesym->mSize)
            {
                mEmitter->Emit("LDA", EAddressingMode::Absolute, exprAddr + currOffset);
                mEmitter->Emit("STA", EAddressingMode::Absolute, stmsym->mAddress + currOffset);
                currOffset++;
            }
        }

        break;
    }
    case EStatementType::ReturnStatement:
    {
        ReturnStatement* retStm = static_cast<ReturnStatement*>(node);

        if (retStm->mExpression != nullptr)
        {
            uint16_t retExprAddr = EmitExpression(retStm->mExpression);

            Symbol* funcSym = mCompilationUnit->mSymbolTable[retStm->mFunction];
            mFuncRetAddrs[funcSym->mUniqueName] = retExprAddr;
        }

        mEmitter->Emit("RTS");

        break;
    }
    default:
        assert(0); // TODO
    }
}

void CodeGenerator::EmitFunction(FunctionDefinition* node)
{
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
        paramSym->mAddrType = ESymAddrType::Relative;
        paramSym->mAddress = RequestVarAddr(paramSym->mSize);

        currParam = static_cast<VarDefStatement*>(currParam->mNext);
    }
    
    // Function body
    Node* currContent = node->mContent;
    while (currContent != nullptr)
    {
        EmitNode(currContent);
        currContent = currContent->mNext;
    }

    funcSym->mSize = mEmitter->GetCurrentLocation() - funcSym->mAddress;
}

void CodeGenerator::EmitStruct(StructDefinition* node)
{
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
    }
}

void CodeGenerator::Generate()
{
    mEmitter->Emit("SEI");
    mEmitter->Emit("CLD");
    // Set stack pointer
    mEmitter->Emit("LDX", EAddressingMode::Immediate, 0xff);
    mEmitter->Emit("TXS");

    Node* currNode = mCompilationUnit->mRootNode;
    while (currNode != nullptr)
    {
        EmitNode(currNode);
        currNode = currNode->mNext;
    }

    // TODO: linking (multiple source files)

    std::ofstream romStream("testrom.nes", std::ios::out | std::ios::binary);

    char* data = mEmitter->GetData();
    // Write header
    data[0] = 'N';
    data[1] = 'E';
    data[2] = 'S';
    data[3] = 0x1a;
    data[4] = 0x01;
    data[5] = 0x01;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    data[12] = 0x00;
    data[13] = 0x00;
    data[14] = 0x00;
    data[15] = 0x00;

    Symbol* mainSym = mCompilationUnit->mSymbolTable["_main"];

    *(int16_t*)&data[0xfffc] = mainSym->mAddress; // reset vector = main function

    memcpy(&data[16], &data[0xc000], 0x4000); // TODO

    // Write program
    romStream.write(data, 0x10000);
    romStream.close();
}
