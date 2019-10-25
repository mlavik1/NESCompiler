#pragma once

#include <string>
#include "tokeniser.h"
#include "compilation_unit.h"
#include "node.h"
#include <unordered_map>
#include <set>
#include <stack>

class Parser
{
private:
    enum EParseResult
    {
        Parsed,
        NotParsed,
        Error
    };

private:
    TokenParser* mTokenParser;
    CompilationUnit* mCompilationUnit;

    EParseResult ParseExpression(Expression** outNode);
    EParseResult ParseExpressionStatement(Node** outNode);
    EParseResult ParseVariableDefinition(Node** outNode);
    EParseResult ParseStatement(Node** outNode);
    EParseResult ParseFunctionDefinition(Node** outNode);
    EParseResult ParseStructDefinition(Node** outNode);
    EParseResult ParseNextNode(Node** outNode);

    void OnError(const std::string& errorString);

public:
    Parser(TokenParser* tokenParser, CompilationUnit* compilationUnit);
    
    void Parse();
};
