#include "parser.h"
#include "analyser.h"

bool Analyser::IsTypeIdentifier(const char* inTokenString)
{
    if (mBuiltInTypes.find(inTokenString) != mBuiltInTypes.end())
        return true;
    else
    {
        return GetSymbol(inTokenString) != nullptr;
    }
}

Symbol* Analyser::GetSymbol(const char* symbolName)
{
    Symbol* currScope = mSymbolTableStack.top();
    while (currScope != nullptr)
    {
        Symbol* currSym = mSymbolTableStack.top()->mContent;
        while (currSym != nullptr)
        {
            if (currSym->mName == symbolName)
                return currSym;
            currSym = currSym->mNext;
        }
        currScope = currScope->mParent;
    }
    return nullptr;
}

void Analyser::AddSymbol(Symbol* symbol)
{
    if (mLastSymbol == nullptr)
    {
        mLastSymbol = symbol;
        mSymbolTableStack.top()->mContent = mLastSymbol;
    }
    else
    {
        mLastSymbol->mNext = symbol;
        mLastSymbol = symbol;
    }
}

void Analyser::PushSybolStack(Symbol* symbol)
{
    mSymbolTableStack.push(symbol);
    mLastSymbol = nullptr;
}

void Analyser::PopSybolStack()
{
    mSymbolTableStack.pop();
    mLastSymbol = mSymbolTableStack.top()->mContent;
    while (mLastSymbol->mNext != nullptr)
        mLastSymbol = mLastSymbol->mNext;
}
