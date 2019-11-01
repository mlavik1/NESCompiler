#include "code_generator.h"

CodeGenerator::CodeGenerator(CompilationUnit* compilationUnit)
{
    mCompilationUnit = compilationUnit;
    emitter = new Emitter();
    mPrgLoc = 0xC000;
    mCurrLoc = mPrgLoc;
}

void CodeGenerator::EmitExpression(Expression* node)
{

}

void CodeGenerator::EmitStatement(Statement* node)
{

}

void CodeGenerator::EmitFunction(FunctionDefinition* node)
{
    Symbol* funcSym = mCompilationUnit->mSymbolTable[node->mName];
    funcSym->mAddress = mCurrLoc;

    // TODO: calculate param addresses (on stack)
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
