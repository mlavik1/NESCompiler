#pragma once
#include <string>

#include "tokeniser.h"

enum class ENodeType
{
    Block,
    Statement,
    FunctionDefinition,
    StructDefinition,
    Expression
};

enum class EStatementType
{
    VariableDefinition, Expression, ReturnStatement, ControlStatement
};

/**
* Node: Anything that can be parsed (statement, expression, etc)
*/
class Node
{
public:
    Node* mNext = nullptr;

    virtual ~Node() {}

    virtual ENodeType GetNodeType() = 0;
};

/**
* A block of code, surrounded by curly brackes.
* Represents a scope
*/
class Block : public Node
{
public:
    Node * mNode = nullptr;

    virtual ENodeType GetNodeType() override { return ENodeType::Block; };
};


/***** EXPRESSIONS *****/

enum class EExpressionType
{
    BinaryOperation, UnaryOperation, Literal, Identifier, FunctionCall
};

enum class EUnaryExpressionType
{
    Prefixx, Postfix
};

enum class EIdentifierType
{
    Variable, // ex: myInt
    StructMember, // ex: structInstance.memberVar
    Namespace // ex: MyStruct.staticVariable
};

/**
* An expression (such as "3", "myFunc()", "a == b", etc.)
*/
class Expression : public Node
{
public:
    std::string mValueType;

    virtual EExpressionType GetExpressionType() const = 0;
    virtual ENodeType GetNodeType() override { return ENodeType::Expression; };
};

class BinaryOperationExpression : public Expression
{
public:
    std::string mOperator;
    Expression* mLeftOperand;
    Expression* mRightOperand;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::BinaryOperation; }
};

class UnaryOperationExpression : public Expression
{
public:
    std::string mOperator;
    Expression* mOperand;
    EUnaryExpressionType mUnaryType;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::UnaryOperation; }
};

class LiteralExpression : public Expression
{
public:
    Token mToken;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::Literal; }
};

class IdentifierExpression : public Expression
{
public:
    std::string mIdentifier;
    EIdentifierType mIdentifierType;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::Identifier; }
};

class FunctionCallExpression : public Expression
{
public:
    std::string mFunction;
    Expression* mParameters = nullptr;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::FunctionCall; }
};

/***** STATEMENTS *****/

class Statement : public Node
{
public:
    virtual EStatementType GetStatementType() const = 0;

    virtual ENodeType GetNodeType() override { return ENodeType::Statement; };
};

class ReturnStatement : public Statement
{
public:
    std::string mFunction; // TODO: do we need this?

    Expression * mExpression = nullptr;

    virtual EStatementType GetStatementType() const override { return EStatementType::ReturnStatement; };
};

/**
* Statement containing only an expression.
*/
class ExpressionStatement : public Statement
{
public:
    std::string mVariableName;
    Expression * mExpression = nullptr;
    virtual EStatementType GetStatementType() const override { return EStatementType::Expression; };
};

/**
* Statement that defines a variable.
*/
class VarDefStatement : public Statement
{
public:
    std::string mType;
    std::string mName;
    Expression* mExpression = nullptr;
    virtual EStatementType GetStatementType() const override { return EStatementType::VariableDefinition; };
};

/**
* Control statement, such as: if(...), else(...), while(...)
*/
class ControlStatement : public Statement
{
    enum class EControlStatement
    {
        If, ElseIf, Else, While
    };
public:
    EControlStatement mControlStatementType;
    Expression* mExpression = nullptr;
    Block* mBody = nullptr;
    virtual EStatementType GetStatementType() const override { return EStatementType::ControlStatement; };
};


class FunctionDefinition : public Node
{
public:
    std::string mType;
    std::string mName;
    VarDefStatement* mParams = nullptr;
    Node* mContent = nullptr;

    virtual ENodeType GetNodeType() override { return ENodeType::FunctionDefinition; };
};

class StructDefinition : public Node
{
public:
    std::string mName;
    Node* mContent = nullptr;

    virtual ENodeType GetNodeType() override { return ENodeType::StructDefinition; };
};
