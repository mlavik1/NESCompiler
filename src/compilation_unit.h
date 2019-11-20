#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "node.h"
#include "relocation.h"

enum class ESymbolType
{
    None = 0,
    Namespace = 1,
    Variable = 2,
    Function = 4,
    Struct = 8,
    FuncParam = 16,
    BuiltInType = 32,
    All = 63
};

inline ESymbolType operator|(ESymbolType a, ESymbolType b)
{
    return static_cast<ESymbolType>(static_cast<int>(a) | static_cast<int>(b));
}

inline ESymbolType operator&(ESymbolType a, ESymbolType b)
{
    return static_cast<ESymbolType>(static_cast<int>(a) & static_cast<int>(b));
}

enum class ESymAddrType
{
    None,
    Absolute,
    Relative
};

class Symbol; // fwd. decl.

class SymbolList
{
public:
    Symbol * mHead = nullptr;
    Symbol* mTail = nullptr;
    Symbol* mOwningSymbol = nullptr;
    SymbolList* mParent = nullptr;
    std::string mName = "";
};

class Symbol
{
public:
    ESymbolType mSymbolType;
    std::string mName;
    std::string mUniqueName;
    // next symbol (in same scope)
    Symbol* mNext = nullptr;
    SymbolList* mChildren = nullptr;
    // type name (of variable/function)
    std::string mTypeName;
    // address type (relative or absolute)
    ESymAddrType mAddrType = ESymAddrType::None; // None = not set
    // type name (of variable/function)
    uint16_t mAddress = 0;
    uint16_t mSize = 0;
};

struct CompilationUnit
{
public:
    std::unordered_map<std::string, Symbol*> mSymbolTable;
    Node* mRootNode;

    std::vector<char> mObjectCode;
    RelocationText mRelocationText;
};
