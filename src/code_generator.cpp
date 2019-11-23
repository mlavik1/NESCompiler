#include "code_generator.h"
#include <assert.h>
#include <sstream>
#include <algorithm>

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

CodeGenerator::CodeGenerator(CompilationUnit* compilationUnit, Emitter* emitter, DataAllocator* dataAllocator)
{
    mCompilationUnit = compilationUnit;
    mEmitter = emitter;
    mDataAllocator = dataAllocator;

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

uint16_t CodeGenerator::Emit(const std::string& op, const EAddressingMode addrMode, const EmitAddr addr)
{
    switch (addr.mType)
    {
    /*case EEmitAddrType::Value:
        return mEmitter->Emit(op.c_str(), addrMode, addr.mValue);
        break;*/
    case EEmitAddrType::DataAddress:
    {
        // Emit
        uint16_t bytesWritten = mEmitter->Emit(op.c_str(), addrMode, addr.mAddress);
        return bytesWritten;
        break;
    }
    case EEmitAddrType::CodeAddress:
    {
        // Get operand value
        uint16_t opVal = addr.mRelativeSymbol != nullptr ? addr.mRelativeSymbol->mAddress + addr.mAddress : addr.mAddress;

        // Emit
        uint16_t bytesWritten = mEmitter->Emit(op.c_str(), addrMode, opVal);
        // Request symbol address relocation, if symbol without available address (external)
        if (addr.mRelativeSymbol != nullptr && addr.mRelativeSymbol->mAddrType == ESymAddrType::None)
            mCompilationUnit->mRelocationText.mSymAddrRefs.push_back({ mEmitter->GetCurrentLocation() - 2, addr.mRelativeSymbol->mUniqueName });
        // Request address relocation, if code address (data addresses are fixed) and no symbol or resolved symbol address is available
        else
            mCompilationUnit->mRelocationText.mRelativeAddresses.push_back(mEmitter->GetCurrentLocation() - 2);
        
        
        return bytesWritten;
        break;
    }
    }
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

EmitAddr CodeGenerator::EmitLiteralExpression(LiteralExpression* litExpr)
{
    if (litExpr->mToken.mTokenType == ETokenType::IntegerLiteral)
    {
        assert(litExpr->mValueType == "uint8_t");
        uint8_t val = static_cast<uint8_t>(litExpr->mToken.mIntValue);
        EmitAddr emitRes;
        // TODO: Simply return value
        //emitRes.mType = EEmitAddrType::Value;
        //emitRes.mValue = val;
        emitRes.mType = EEmitAddrType::DataAddress;
        emitRes.mAddress = mDataAllocator->RequestVarAddr(1);
        mEmitter->Emit("LDA", EAddressingMode::Immediate, val); // TODO
        Emit("STA", EAddressingMode::Absolute, EmitAddr(EEmitAddrType::DataAddress, emitRes.mAddress, nullptr));
        return emitRes;
    }
    else
        assert(0); // TODO: implement other types
}

EmitAddr CodeGenerator::EmitIdentifierExpression(IdentifierExpression* identExpr)
{
    Symbol* identSym = mCompilationUnit->mSymbolTable[identExpr->mIdentifier];
    return EmitAddr(EEmitAddrType::DataAddress, 0, identSym);
}

EmitAddr CodeGenerator::EmitFuncCallExpression(FunctionCallExpression* callExrp)
{
    Symbol* funcSym = mCompilationUnit->mSymbolTable[callExrp->mFunction];

    // Set parameters
    Symbol* paramSym = funcSym->mChildren ? funcSym->mChildren->mTail : nullptr;
    Expression* paramExpr = callExrp->mParameters;
    while (paramExpr != nullptr)
    {
        // Parameter value expression
        EmitAddr paramExprAddr = EmitExpression(paramExpr);

        // Copy byte by byte
        uint16_t currOffset = 0;
        while (currOffset < paramSym->mSize)
        {
            // Load value from expression into A register
            EmitAddr exprChunkAddr = paramExprAddr;
            exprChunkAddr.mAddress += currOffset;
            Emit("LDA", EAddressingMode::Absolute, exprChunkAddr);
            // Store value of A in parameter symbol address
            EmitAddr currParamAddr(EEmitAddrType::DataAddress, currOffset, paramSym); // relative to sym addr (relocated later)
            Emit("STA", EAddressingMode::Absolute, currParamAddr);
            currOffset++;
        }

        paramExpr = static_cast<Expression*>(paramExpr->mNext);
        paramSym = paramSym->mNext;
    }

    // Jump
    EmitAddr jmpAddr(EEmitAddrType::CodeAddress, funcSym->mAddress, funcSym);
    Emit("JSR", EAddressingMode::Absolute, jmpAddr);

    // Return value
    if (funcSym->mTypeName != "void")
    {
        EmitAddr funcRetAddr = mFuncRetAddrs[funcSym->mUniqueName];

        return funcRetAddr;
    }
    else
    {
        EmitAddr voidAddr;
        voidAddr.mType = EEmitAddrType::None;
        return voidAddr;
    }
}

EmitAddr CodeGenerator::EmitBinOpExpression(BinaryOperationExpression* binOpExpr)
{
    Symbol* valSym = mCompilationUnit->mSymbolTable[binOpExpr->mValueType];

    EmitAddr retAddr;
    retAddr.mType = EEmitAddrType::DataAddress;
    retAddr.mAddress = mDataAllocator->RequestVarAddr(valSym->mSize);

    EmitAddr leftExprAddr = EmitExpression(binOpExpr->mLeftOperand);
    EmitAddr rightExprAddr = EmitExpression(binOpExpr->mRightOperand);

    if (binOpExpr->mValueType == "uint8_t")
    {
        if (binOpExpr->mOperator == "+" || binOpExpr->mOperator == "-")
        {
            Emit("LDA", EAddressingMode::Absolute, leftExprAddr);
            if(binOpExpr->mOperator == "+")
                Emit("ADC", EAddressingMode::Absolute, rightExprAddr); // +
            else
                Emit("SBC", EAddressingMode::Absolute, rightExprAddr); // -
            Emit("STA", EAddressingMode::Absolute, retAddr);
        }
        else if (binOpExpr->mOperator == "==" || binOpExpr->mOperator == "!=")
        {
            Emit("LDA", EAddressingMode::Absolute, leftExprAddr);
            Emit("CMP", EAddressingMode::Absolute, rightExprAddr);
            // Branch
            uint16_t branchAddr = mEmitter->GetCurrentLocation();
            if(binOpExpr->mOperator == "==")
                mEmitter->Emit("BEQ", EAddressingMode::Immediate, 0); // dummy address (0) is relocated below
            else if(binOpExpr->mOperator == "!=")
                mEmitter->Emit("BNE", EAddressingMode::Immediate, 0); // dummy address (0) is relocated below
            // TODO: ">"  "<" (BMI)  ">=" (BPL)  "<="

            // False case
            Emit("LDA", EAddressingMode::Immediate, EmitAddr(EEmitAddrType::DataAddress, 0, nullptr));
            uint16_t jmpAddr = mEmitter->GetCurrentLocation();
            Emit("JMP", EAddressingMode::Absolute, EmitAddr(EEmitAddrType::CodeAddress, 0, nullptr));
            // True case
            uint16_t branchDest = mEmitter->GetCurrentLocation();
            mEmitter->Emit("LDA", EAddressingMode::Immediate, 1);
            uint16_t jmpDest = mEmitter->GetCurrentLocation();

            // Relocate branch/jump destination addresses
            const uint16_t branchNextLoc = branchAddr + 2; // instruction after branch
            const uint8_t displacement = (branchDest >= branchNextLoc) ? (branchDest - branchNextLoc) : (branchNextLoc - branchDest) | 0b10000000;
            mEmitter->EmitDataAtPos(branchAddr + 1, reinterpret_cast<const char*>(&displacement), sizeof(displacement));
            mEmitter->EmitDataAtPos(jmpAddr + 1, reinterpret_cast<const char*>(&jmpDest), sizeof(jmpDest));

            // Write result
            Emit("STA", EAddressingMode::Absolute, EmitAddr(EEmitAddrType::DataAddress, retAddr.mAddress, nullptr)); // TODO: use Emit(...)
        }
    }
    else
        assert(0); // TODO: Implement other types

    return retAddr;
}

EmitAddr CodeGenerator::EmitExpression(Expression* node)
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
    uint16_t codeAddrStart = mEmitter->GetCurrentLocation();

    // Emit condition expression
    EmitAddr exprAddr = EmitExpression(node->mExpression);

    Emit("LDA", EAddressingMode::Absolute, exprAddr);

    uint16_t condBranchLoc = mEmitter->GetCurrentLocation();

    mEmitter->Emit("BEQ", EAddressingMode::Immediate, 0); // relocated below

    // Emit body content
    EmitNode(node->mBody);

    uint16_t branchDest = mEmitter->GetCurrentLocation();

    // TODO: Use JMP if displacement is exceeded
    if (std::abs(branchDest - condBranchLoc) > 127)
        printf("ERROR: Control statement body too large. Max displacement exceeded.");

    // Displacement: Distance offset
    // = abs(b-a) | 0b10000000 -> absolute offset, with first bit as sign bit
    const uint16_t branchNextLoc = condBranchLoc + 2;
    const uint8_t displacement = (branchDest >= branchNextLoc) ? (branchDest - branchNextLoc) : (branchNextLoc - branchDest) | 0b10000000;

    // Relocate branch destination address
    mEmitter->EmitDataAtPos(condBranchLoc + 1, reinterpret_cast<const char*>(&displacement), sizeof(uint8_t));

    // else { ... }
    if (node->mControlStatementType == ControlStatement::EControlStatementType::If && node->mConnectedStatement != nullptr)
    {
        EmitNode(node->mConnectedStatement);
    }

    // while loop: jump back to condition check
    if (node->mControlStatementType == ControlStatement::EControlStatementType::While)
    {
        Emit("JMP", EAddressingMode::Absolute, EmitAddr(EEmitAddrType::CodeAddress, condBranchLoc, nullptr));
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
            stmsym->mAddress = mDataAllocator->RequestVarAddr(typesym->mSize);
            stmsym->mSize = typesym->mSize; // ??
        }

        if (varDefStm->mExpression != nullptr)
        {
            assert(varDefStm->mExpression->mValueType == varDefStm->mType);

            EmitAddr exprAddr = EmitExpression(varDefStm->mExpression);

            // Copy byte by byte
            uint16_t currOffset = 0;
            while (currOffset < typesym->mSize)
            {
                // Load value from expression into A register
                EmitAddr exprChunkAddr = exprAddr;
                exprChunkAddr.mAddress += currOffset;
                Emit("LDA", EAddressingMode::Absolute, exprChunkAddr);

                // Store value of A in parameter symbol address
                EmitAddr varChunkAddr(EEmitAddrType::DataAddress, currOffset, stmsym);
                Emit("STA", EAddressingMode::Absolute, varChunkAddr);
                currOffset++;
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
            EmitAddr retExprAddr = EmitExpression(retStm->mExpression);

            Symbol* funcSym = mCompilationUnit->mSymbolTable[retStm->mFunction];
            mFuncRetAddrs[funcSym->mUniqueName] = retExprAddr;
        }

        mEmitter->Emit("RTS");

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
        paramSym->mAddrType = ESymAddrType::Relative;
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
        mEmitter->Emit("RTS");

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
        const std::string opValStr = node->mOp1.substr((isVal ? 1 : 0) + (isHex ? 1 : 0));

        assert(isSym || isVal || isHex);

        // Get operand value
        unsigned int opVal;
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
        else
            opVal = std::stoi(opValStr);

        // Get addressing mode
        if (isVal)
        {
            addrMode = EAddressingMode::Immediate;
        }
        else if (isSym)
        {
            addrMode = EAddressingMode::Absolute;
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
