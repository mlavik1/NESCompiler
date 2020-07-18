#pragma once

#include <string>
#include "tokeniser.h"
#include "compilation_unit.h"
#include "node.h"
#include <unordered_map>
#include <set>
#include <stack>
#include "operator.h"

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

    std::unordered_map<std::string, OperatorInfo> mUnaryPrefixOperatorsMap;
    std::unordered_map<std::string, OperatorInfo> mUnaryPostfixOperatorsMap;
    std::unordered_map<std::string, OperatorInfo> mBinaryOperatorsMap;
	std::set<std::string> mNoOperandOpcodes;

    OperatorInfo mDefaultOuterOperatorInfo = { "", 999, EOperatorAssociativity::LeftToRight };

    EParseResult ParseBinaryOperator(OperatorInfo& outOperator);
    EParseResult ParseUnaryPostfixOperator(OperatorInfo& outOperator);
    EParseResult ParseUnaryPrefixOperator(OperatorInfo& outOperator);
    EParseResult ParseAtom(Expression** outExpression);

    EParseResult ParseExpression(const OperatorInfo& inOperator, Expression** outExpression);
    EParseResult ParseExpressionStatement(Node** outNode);
    EParseResult ParseBlock(Node** outNode);
    EParseResult ParseReturnStatement(Node** outNode);
    EParseResult ParseVariableDefinition(Node** outNode);
    EParseResult ParseElseStatement(Node** outNode);
    EParseResult ParseControlStatement(Node** outNode);
    EParseResult ParseStatement(Node** outNode);
    EParseResult ParseFunctionDefinition(Node** outNode);
    EParseResult ParseStructDefinition(Node** outNode);
    EParseResult ParseInlineAssembly(Node** outNode);
    EParseResult ParseNextNode(Node** outNode);

    void OnError(const std::string& errorString);

public:
    Parser(TokenParser* tokenParser, CompilationUnit* compilationUnit);
    
    void Parse();
};
