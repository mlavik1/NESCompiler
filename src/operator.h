#pragma once

#include <string>

enum class EOperatorAssociativity
{
    LeftToRight,
    RightToLeft
};

class OperatorInfo
{
public:
    std::string mOperator;
    int mPrecedence;
    EOperatorAssociativity mAssociativity;
};
