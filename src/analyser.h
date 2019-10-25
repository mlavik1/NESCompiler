#pragma once

#include <string>
#include "compilation_unit.h"
#include "node.h"
#include <unordered_map>
#include <set>
#include <stack>

class Analyser
{
private:
    CompilationUnit* mCompilationUnit;
    Symbol* mRootSymbol;
    std::stack<Symbol*> mSymbolTableStack;
    Symbol* mLastSymbol;

    std::set<std::string> mBuiltInTypes;

    bool IsTypeIdentifier(const char* inTokenString);
    Symbol* GetSymbol(const char* symbolName);
    void AddSymbol(Symbol* symbol);
    void PushSybolStack(Symbol* symbol);
    void PopSybolStack();
};
