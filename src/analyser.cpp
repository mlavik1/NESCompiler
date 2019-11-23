#include "parser.h"
#include "analyser.h"
#include "debug.h"
#include <assert.h>

Analyser::Analyser(CompilationUnit* unit)
{
    mCompilationUnit = unit;

    mBuiltInTypes.emplace("uint8_t");
    mBuiltInTypes.emplace("void");
}

bool Analyser::IsTypeIdentifier(const char* inTokenString)
{
    if (mBuiltInTypes.find(inTokenString) != mBuiltInTypes.end())
        return true;
    else
    {
        return GetSymbol(inTokenString, ESymbolType::All) != nullptr;
    }
}

Symbol* Analyser::GetSymbol(const std::string& symbolName, ESymbolType symbolType)
{
    SymbolList* symList = mCurrentScope;
    while (symList != nullptr)
    {
        Symbol* currSym = symList->mTail;
        while (currSym != nullptr)
        {
            if (((currSym->mSymbolType & symbolType) != ESymbolType::None) && currSym->mName == symbolName)
                return currSym;
            currSym = currSym->mNext;
        }
        symList = symList->mParent;
    }
    return nullptr;
}

void Analyser::PushSybolStack(Symbol* symbol)
{
    SymbolList* symList = new SymbolList();
    symList->mHead = symList->mTail = nullptr;
    symList->mParent = mCurrentScope;
    symList->mName = mCurrentScope->mName + std::string("_") + symbol->mName;
    symList->mOwningSymbol = symbol;
    symbol->mChildren = symList;
    mCurrentScope = symList;
}

void Analyser::PopSybolStack()
{
    mCurrentScope = mCurrentScope->mParent;
}

void Analyser::AddSymbol(Symbol* symbol)
{
    SymbolList* symList = mCurrentScope;
    if (symList->mHead == nullptr)
    {
        symList->mHead = symList->mTail = symbol;
    }
    else
    {
        symList->mHead->mNext = symbol;
        symList->mHead = symbol;
    }
}

void Analyser::GenerateUniqueName(Symbol* sym)
{
    sym->mUniqueName = mCurrentScope->mName + std::string("_") + sym->mName;
}

bool Analyser::ConvertTypeName(const std::string& typeName, std::string& outUniqueName)
{
    Symbol* typeSym = GetSymbol(typeName, ESymbolType::Struct);
    if (typeSym != nullptr)
        outUniqueName = typeSym->mUniqueName;
    else if (mBuiltInTypes.find(typeName) != mBuiltInTypes.end()) // TODO: should built'in types have symbols?
        outUniqueName = typeName;
    else
    {
        LOG_ERROR() << "Invalid type type: " << typeName;
        OnError();
        return false;
    }
    return true;
}

Symbol* Analyser::VisitBlockNode(Block* node)
{
    Node* currentNode = node->mNode;
    while (currentNode != nullptr)
    {
        VisitNode(currentNode);
        currentNode = currentNode->mNext;
    }
    return nullptr;
}

Symbol* Analyser::VisitStructDefNode(StructDefinition* node)
{
    Symbol* sym = GetSymbol(node->mName, ESymbolType::Struct);

    // Create symbol
    if (sym == nullptr)
    {
        sym = new Symbol();
        sym->mName = node->mName;
        sym->mSymbolType = ESymbolType::Struct;
        // Generate unique name
        GenerateUniqueName(sym);

        AddSymbol(sym);
    }
    // Update node name
    node->mName = sym->mUniqueName;

    if (sym->mChildren != nullptr && node->mContent != nullptr)
    {
        LOG_ERROR() << "Struct already defined: " << node->mName;
        OnError();
    }

    // Visit content nodes
    if (node->mContent != nullptr)
    {
        PushSybolStack(sym);

        Node* currContent = node->mContent;
        while (currContent != nullptr)
        {
            VisitNode(currContent);
            currContent = currContent->mNext;
        }

        PopSybolStack();
    }

    return sym;
}

Symbol* Analyser::VisitFuncDefNode(FunctionDefinition* node)
{
    Symbol* sym = GetSymbol(node->mName, ESymbolType::Function);

    // Create symbol
    if (sym == nullptr)
    {
        sym = new Symbol();
        sym->mName = node->mName;
        sym->mSymbolType = ESymbolType::Function;
        // Generate unique name
        GenerateUniqueName(sym);
        // Convert type name
        if (!ConvertTypeName(node->mType, sym->mTypeName))
            return nullptr;
        AddSymbol(sym);
    }
    // Update name
    node->mName = sym->mUniqueName;
    node->mType = sym->mTypeName;

    if (sym->mChildren != nullptr && node->mContent != nullptr)
    {
        LOG_ERROR() << "Function already defined: " << node->mName;
        OnError();
        return nullptr;
    }

    if (mCurrentScope->mOwningSymbol != nullptr && (mCurrentScope->mOwningSymbol->mSymbolType & (ESymbolType::Struct | ESymbolType::Namespace)) == ESymbolType::None)
    {
        LOG_ERROR() << "Can't declare function " << node->mName << " in this scope";
        OnError();
        return nullptr;
    }

    if (node->mContent != nullptr)
    {
        PushSybolStack(sym);

        // Visit param nodes
        VarDefStatement* currParam = node->mParams;
        while (currParam != nullptr)
        {
            Symbol* paramSym = VisitStatementNode(currParam);
            paramSym->mSymbolType = ESymbolType::FuncParam;
            currParam = static_cast<VarDefStatement*>(currParam->mNext);
        }
        // Visit content nodes
        Node* currContent = node->mContent;
        while (currContent != nullptr)
        {
            VisitNode(currContent);
            currContent = currContent->mNext;
        }

        PopSybolStack();
    }

    // TODO: Check that (non-void) function returns something

    return sym;
}

Symbol* Analyser::VisitExpressionStatement(ExpressionStatement* node)
{
    VisitExpression(node->mExpression);
    return nullptr; // ???
}

Symbol* Analyser::VisitVarDefStatement(VarDefStatement* node)
{
    Symbol* sym = GetSymbol(node->mName, ESymbolType::Function);

    // Create symbol
    if (sym == nullptr)
    {
        sym = new Symbol();
        sym->mName = node->mName;
        sym->mSymbolType = ESymbolType::Variable;
        AddSymbol(sym);
        // Generate unique name
        GenerateUniqueName(sym);
        // Convert type name
        if (!ConvertTypeName(node->mType, sym->mTypeName))
            return nullptr;
    }
    // Update node name and type
    node->mName = sym->mUniqueName;
    node->mType = sym->mTypeName;

    if (node->mExpression != nullptr)
    {
        VisitExpression(node->mExpression);
        if (node->mExpression->mValueType != node->mType)
        {
            LOG_ERROR() << "Type mismatch in variable definition: " << node->mType << " " << node->mName << " and " << node->mExpression->mValueType;
            OnError();
        }
    }
    return sym;
}

Symbol* Analyser::VisitControlStatement(ControlStatement* node)
{
    VisitExpression(node->mExpression);
    
    Node* currNode = node->mBody;
    while (currNode != nullptr)
    {
        VisitNode(node->mBody);
        currNode = currNode->mNext;
    }

    return nullptr;
}

Symbol* Analyser::VisitStatementNode(Statement* node)
{
    EStatementType statementType = node->GetStatementType();
    switch (statementType)
    {
    case EStatementType::VariableDefinition:
    {
        return VisitVarDefStatement((VarDefStatement*)node);
        break;
    }
    case EStatementType::ControlStatement:
    {
        return VisitControlStatement((ControlStatement*)node);
        break;
    }
    case EStatementType::ReturnStatement:
    {
        ReturnStatement* retStm = static_cast<ReturnStatement*>(node);
        if (mCurrentScope->mOwningSymbol->mSymbolType != ESymbolType::Function)
        {
            LOG_ERROR() << "Return statement can only exist inside a function";
            OnError();
        }
        else
            retStm->mFunction = mCurrentScope->mOwningSymbol->mUniqueName;

        if (retStm->mExpression != nullptr)
            VisitExpression(retStm->mExpression);

        break;
    }
    case EStatementType::Expression:
    {
        return VisitExpressionStatement((ExpressionStatement*)node);
        break;
    }
    default:
        LOG_ERROR() << "Unhandled statement type: " << (int)statementType;
        OnError();
    }
    return nullptr;
}

void Analyser::VisitInlineAssemblyNode(InlineAssemblyStatement* node)
{
    if (node->mOp1 != "")
    {
        Symbol* varSym = GetSymbol(node->mOp1, ESymbolType::Variable);
        if (varSym != nullptr)
        {
            node->mOp1 = varSym->mUniqueName;
        }
    }
}

void Analyser::VisitExpression(Expression* node)
{
    EExpressionType exprType = node->GetExpressionType();
    switch (exprType)
    {
    case EExpressionType::BinaryOperation:
    {
        BinaryOperationExpression* binOpExpr = (BinaryOperationExpression*)node;
        VisitExpression(binOpExpr->mLeftOperand);
        VisitExpression(binOpExpr->mRightOperand);
        if (binOpExpr->mLeftOperand->mValueType != binOpExpr->mRightOperand->mValueType)
        {
            LOG_ERROR() << "Binary operation expression type mismatch: " << binOpExpr->mLeftOperand->mValueType << binOpExpr->mOperator << binOpExpr->mRightOperand->mValueType;
            OnError();
        }
        else
            node->mValueType = binOpExpr->mLeftOperand->mValueType;
        break;
    }
    case EExpressionType::FunctionCall:
    {
        FunctionCallExpression* funcCallExpr = (FunctionCallExpression*)node;
        Symbol* funcSym = GetSymbol(funcCallExpr->mFunction, ESymbolType::Function);
        funcCallExpr->mFunction = funcSym->mUniqueName;

        Expression* currParamExpr = funcCallExpr->mParameters;
        Symbol* currParamSym = funcSym->mChildren ? funcSym->mChildren->mTail : nullptr;
        while(currParamExpr != nullptr)
        {
            if (currParamSym == nullptr || currParamSym->mSymbolType != ESymbolType::FuncParam)
            {
                LOG_ERROR() << "Invalid function call parameters to function: " << funcCallExpr->mFunction;
                OnError();
                break;
            }
            // Visit expression node
            VisitExpression(static_cast<Expression*>(currParamExpr));
            // Check param value type
            if (currParamExpr->mValueType != currParamSym->mTypeName)
            {
                LOG_ERROR() << "Function call parameter type mismatch: " << currParamExpr->mValueType << " and " << currParamSym->mTypeName;
                OnError();
                break;
            }

            currParamExpr = (Expression*)currParamExpr->mNext;
            currParamSym = currParamSym->mNext;
        }
        
        node->mValueType = funcSym->mTypeName;

        break;
    }
    case EExpressionType::Identifier:
    {
        IdentifierExpression* identExpr = (IdentifierExpression*)node;
        
        Symbol* identSym = GetSymbol(identExpr->mIdentifier, ESymbolType::Variable | ESymbolType::FuncParam);
        if (identSym == nullptr)
        {
            LOG_ERROR() << "Undeclared identifier: " << identExpr->mIdentifier;
            OnError();
        }
        else
        {
            identExpr->mIdentifier = identSym->mUniqueName;
            node->mValueType = identExpr->mValueType = identSym->mTypeName;
        }

        break;
    }
    case EExpressionType::Literal:
    {
        LiteralExpression* litExpr = (LiteralExpression*)node;
        if (litExpr->mToken.mTokenType == ETokenType::IntegerLiteral)
            node->mValueType = litExpr->mValueType = "uint8_t";
        else
        {
            LOG_ERROR() << "Invalid literal type: " << litExpr->mToken.mTokenString; // TODO
        }
        break;
    }
    case EExpressionType::UnaryOperation:
    {
        UnaryOperationExpression* unOpExpr = (UnaryOperationExpression*)node;
        break;
    }
    default:
        LOG_ERROR() << "Unhandled expression type: " << (int)exprType;
        OnError();
        return;
    }
}

void Analyser::VisitNode(Node* node)
{
    ENodeType nodeType = node->GetNodeType();
    switch (nodeType)
    {
    case ENodeType::Block:
    {
        VisitBlockNode(reinterpret_cast<Block*>(node));
        break;
    }
    case ENodeType::StructDefinition:
    {
        VisitStructDefNode(reinterpret_cast<StructDefinition*>(node));
        break;
    }
    case ENodeType::FunctionDefinition:
    {
        VisitFuncDefNode(reinterpret_cast<FunctionDefinition*>(node));
        break;
    }
    case ENodeType::Statement:
    {
        VisitStatementNode(reinterpret_cast<Statement*>(node));
        break;
    }
    case ENodeType::InlineAssembly:
    {
        VisitInlineAssemblyNode(reinterpret_cast<InlineAssemblyStatement*>(node));
        break;
    }
    default:
    {
        LOG_ERROR() << "Unhandled ENodeType: " << (int)nodeType;
        OnError();
        return;
    }
    }
}

void Analyser::RegisterSymbolRecursive(Symbol* sym)
{
    assert(mCompilationUnit->mSymbolTable.find(sym->mUniqueName) == mCompilationUnit->mSymbolTable.end());
    mCompilationUnit->mSymbolTable.emplace(sym->mUniqueName, sym);

    if (sym->mChildren != nullptr)
    {
        Symbol* currSym = sym->mChildren->mTail;
        while (currSym != nullptr)
        {
            RegisterSymbolRecursive(currSym);
            currSym = currSym->mNext;
        }
    }
}

void Analyser::Analyse()
{
    mCurrentScope = mSymbolList = new SymbolList();
    mSymbolList->mHead = mSymbolList->mTail = nullptr;

    Node* currNode = mCompilationUnit->mRootNode;
    while (currNode != nullptr)
    {
        VisitNode(currNode);
        currNode = currNode->mNext;
    }

    Symbol* currSym = mSymbolList->mTail;
    while (currSym != nullptr)
    {
        RegisterSymbolRecursive(currSym);
        currSym = currSym->mNext;
    }

    LOG_INFO() << "*** SYMBOL TABLE: ***";
    for (auto sym : mCompilationUnit->mSymbolTable)
    {
        LOG_INFO() << sym.first << " : " << sym.second->mTypeName;
    }
}

void Analyser::OnError()
{
    mFailed = true;
}
