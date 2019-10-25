#pragma once
#include <string>

#include "tokeniser.h"

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
};

/**
* A block of code, surrounded by curly brackes.
* Represents a scope
*/
class Block : public Node
{
public:
    Node * mNode = nullptr;
};


/***** EXPRESSIONS *****/

enum class EExpressionType
{
    BinaryOperation, UnaryOperation, Literal, VariableAccess, FunctionCall
};

/**
* An expression (such as "3", "myFunc()", "a == b", etc.)
*/
class Expression : public Node
{
public:
    std::string mValueType; // TODO
    virtual EExpressionType GetExpressionType() const = 0;
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
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::UnaryOperation; }
};

class LiteralExpression : public Expression
{
public:
    Token mToken;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::Literal; }
};

class VariableAccessExpression : public Expression
{
public:
    std::string mVariable;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::VariableAccess; }
};

class FunctionCallExpression : public Expression
{
public:
    std::string mIdentifier;
    Expression* mParameters = nullptr;
    virtual EExpressionType GetExpressionType() const override { return EExpressionType::FunctionCall; }
};

/***** STATEMENTS *****/

class Statement : public Node
{
public:
    virtual EStatementType GetStatementType() const = 0;
};

class ReturnStatement : public Statement
{
public:
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
    std::string mVariableType;
    std::string mVariableName;
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
    Node* mParams = nullptr;
    Node* mContent = nullptr;
};

class StructDefinition : public Node
{
public:
    std::string mName;
    Node* mContent = nullptr;
};
