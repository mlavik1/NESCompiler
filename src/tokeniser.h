#pragma once

#include <vector>
#include <set>
#include <unordered_map>
#include <stack>

enum class ETokenType
{
    EndOfFile,
    FloatLiteral,
    IntegerLiteral,
    BooleanLiteral,
    Operator,
    PreprocessorDirective,
    Identifier,
    StringLiteral,
    NewLine
};

class Token
{
public:
    ETokenType mTokenType;
    std::string mTokenString;
    float mFloatValue;
    int mIntValue;
    int mLineNumber;
};

class Tokeniser
{
private:
    const std::set<char> mPunctuators = { '[', ']', '(' , ')' , '{' , '}' , ',' , '.' , ';' , ':' , '<', '>', '=', '!', '+', '-', '*', '/', '&', '|', '?' };
    const std::set<std::string> mDoublePunctuators = { "==", ">=", "<=", "!=", "&&", "||", "+=", "*=", "/=", "&=", "|=", "->" };

    std::string mSourceText;
    const char* mSourceStringPos;
    int mLineNumber = 1;

    bool isOperator(const char* inToken);

public:
    Tokeniser(const char* inSourceText);

    Token ParseToken();
};

class TokenParser
{
private:
    Tokeniser mTokeniser;
    std::vector<Token> mTokens;
    size_t mCurrentTokenIndex = 0;

public:
    TokenParser(const char* inShaderCode);
    void ResetPosition();
    void Advance();
    const Token& GetCurrentToken();
    const Token& GetTokenFromOffset(const int inOffset);
    bool HasMoreTokens();
    void SetTokens(std::vector<Token>& inTokens);

    size_t GetCurrentTokenIndex() { return mCurrentTokenIndex; };
    std::vector<Token>& GetTokens() { return mTokens; };
};
