#pragma once

#include <string>
#include "compilation_unit.h"
#include "node.h"
#include <unordered_map>
#include <set>
#include <stack>
#include "compilation_unit.h"

class Analyser
{
private:
    CompilationUnit* mCompilationUnit;
    SymbolList* mSymbolList;
    SymbolList* mCurrentScope;
    std::set<std::string> mBuiltInTypes;
    bool mFailed = false;

    bool IsTypeIdentifier(const char* inTokenString);
    Symbol* GetSymbol(const std::string& symbolName, ESymbolType symbolType);
    void PushSybolStack(Symbol* symbol);
    void PopSybolStack();
    void AddSymbol(Symbol* symbol);
    
    void GenerateUniqueName(Symbol* sym);
    bool ConvertTypeName(const std::string& typeName, std::string& outUniqueName);

    Symbol* VisitBlockNode(Block* node);
    Symbol* VisitStructDefNode(StructDefinition* node);
    Symbol* VisitFuncDefNode(FunctionDefinition* node);
    Symbol* VisitExpressionStatement(ExpressionStatement* node);
    Symbol* VisitVarDefStatement(VarDefStatement* node);
    Symbol* VisitControlStatement(ControlStatement* node);
    Symbol* VisitStatementNode(Statement* node);
    void VisitInlineAssemblyNode(InlineAssemblyStatement* node);
    void VisitExpression(Expression* node);
    void VisitNode(Node* node);

    void RegisterSymbolRecursive(Symbol* sym);

    void OnError();

public:
    Analyser(CompilationUnit* unit);

    void Analyse();
};
