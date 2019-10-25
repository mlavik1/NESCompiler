#pragma once

#include <unordered_map>
#include <string>
#include <vector>

enum class ESymbolType
{
    Namespace,
    Variable,
    Function,
    Struct
};

class Symbol
{
public:
    ESymbolType mSymbolType;
    std::string mName;
    std::string mUniqueName;
    //  next symbol (in same scope)
    Symbol* mNext = nullptr;
    // parent symbol (parent scope)
    Symbol* mParent = nullptr;
    // content symbols (child symbols of struct/function)
    Symbol* mContent = nullptr;
    // type name (of variable/function)
    std::string mTypeName;
    // type name (of variable/function)
    size_t mAddress = 0;
};

struct CompilationUnit
{
public:
    Symbol* mRootSymbol;
};
