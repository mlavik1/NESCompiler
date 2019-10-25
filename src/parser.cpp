#include "parser.h"
#include "debug.h"

Parser::Parser(TokenParser* tokenParser, CompilationUnit* compilationUnit)
{
    mTokenParser = tokenParser;
    mCompilationUnit = compilationUnit;
}

Parser::EParseResult Parser::ParseExpression(Expression** outNode)
{
    return EParseResult::NotParsed;
}

Parser::EParseResult Parser::ParseExpressionStatement(Node** outNode)
{
    const Token nameToken = mTokenParser->GetTokenFromOffset(1);
    const Token secondToken = mTokenParser->GetTokenFromOffset(2);

    if (nameToken.mTokenType != ETokenType::Identifier || secondToken.mTokenString != "=")
        return EParseResult::NotParsed;

    mTokenParser->Advance();
    mTokenParser->Advance();

    // Create node
    ExpressionStatement* varDefNode = new ExpressionStatement();
    varDefNode->mVariableName = nameToken.mTokenString;
    *outNode = varDefNode;

    // Parse assignment expression
    EParseResult exprResult = ParseExpression(&varDefNode->mExpression);
    if (exprResult != EParseResult::Parsed)
    {
        OnError("Invalid variable assignment expression.");
        return EParseResult::Error;
    }

    return EParseResult::Parsed;
}

Parser::EParseResult Parser::ParseVariableDefinition(Node** outNode)
{
    const Token typeToken = mTokenParser->GetCurrentToken();
    const Token nameToken = mTokenParser->GetTokenFromOffset(1);
    const Token thirdToken = mTokenParser->GetTokenFromOffset(2);

    if (typeToken.mTokenType != ETokenType::Identifier || nameToken.mTokenType != ETokenType::Identifier)
        return EParseResult::NotParsed;
    if(thirdToken.mTokenString != "=" && thirdToken.mTokenString != ";")
        return EParseResult::NotParsed;

    mTokenParser->Advance();
    mTokenParser->Advance();
    mTokenParser->Advance();

    // Create node
    VarDefStatement* varDefNode = new VarDefStatement();
    varDefNode->mVariableType = typeToken.mTokenString;
    varDefNode->mVariableName = nameToken.mTokenString;
    *outNode = varDefNode;

    // Only declaration?
    if (thirdToken.mTokenString == ";")
        return EParseResult::Parsed;

    mTokenParser->Advance();

    // Parse assignment expression
    EParseResult exprResult = ParseExpression(&varDefNode->mExpression);
    if (exprResult != EParseResult::Parsed)
    {
        OnError("Invalid variable assignment expression.");
        return EParseResult::Error;
    }

    return EParseResult::Parsed;
}

Parser::EParseResult Parser::ParseStatement(Node** outNode)
{
    // Variable definition
    EParseResult parseResult = ParseVariableDefinition(outNode);
    if (parseResult != EParseResult::NotParsed)
    {
        return parseResult;
    }

    // Expression statement
    parseResult = ParseExpressionStatement(outNode);
    if (parseResult != EParseResult::NotParsed)
    {
        return parseResult;
    }

    return EParseResult::NotParsed;
}

Parser::EParseResult Parser::ParseFunctionDefinition(Node** outNode)
{
    const Token typeToken = mTokenParser->GetCurrentToken();
    if (typeToken.mTokenType != ETokenType::Identifier)
        return EParseResult::NotParsed;
    
    const Token nameToken = mTokenParser->GetTokenFromOffset(1);
    if (nameToken.mTokenType != ETokenType::Identifier)
        return EParseResult::NotParsed;

    if (mTokenParser->GetTokenFromOffset(2).mTokenString != "(")
        return EParseResult::NotParsed;

    mTokenParser->Advance(); // name
    mTokenParser->Advance(); // (
    mTokenParser->Advance(); // first param, or )

    FunctionDefinition* funcDefNode = new FunctionDefinition();
    *outNode = funcDefNode;
    funcDefNode->mType = typeToken.mTokenString;
    funcDefNode->mName = nameToken.mTokenString;

    // Parse function parameters
    Node** currParamNode = &funcDefNode->mParams;
    while (mTokenParser->GetCurrentToken().mTokenString != ")")
    {
        const Token paramIdentifier = mTokenParser->GetCurrentToken();
        const Token paramName = mTokenParser->GetTokenFromOffset(1);
        const Token paramDelimiter = mTokenParser->GetTokenFromOffset(2);

        if (paramIdentifier.mTokenType != ETokenType::Identifier || paramName.mTokenType != ETokenType::Identifier
            || (paramDelimiter.mTokenString != "," && paramDelimiter.mTokenString != ")"))
        {
            OnError("Invalid function parameter");
            return EParseResult::Error;
        }

        VarDefStatement* param = new VarDefStatement();
        param->mVariableType = paramIdentifier.mTokenString;
        param->mVariableName = paramName.mTokenString;
        *currParamNode = param;
        currParamNode = &param->mNext;
    }

    mTokenParser->Advance();

    // Function declaration only?
    if (mTokenParser->GetCurrentToken().mTokenString == ";")
    {
        return EParseResult::Parsed;
    }

    // Parse function body
    if (mTokenParser->GetCurrentToken().mTokenString == "{")
    {
        mTokenParser->Advance();
        Node** currNodePtr = &funcDefNode->mContent;
        while (mTokenParser->GetCurrentToken().mTokenString != "}")
        {
            EParseResult contentParseResult = ParseNextNode(currNodePtr);
            if (contentParseResult != EParseResult::Parsed)
            {
                // TODO: delete node ???
                OnError("Failed to parse function body.");
                return EParseResult::Error;
            }
        }
    }
    else
    {
        // TODO: Delete node ???
        OnError("Expected { but found: " + mTokenParser->GetCurrentToken().mTokenString);
        return EParseResult::Error;
    }

    mTokenParser->Advance();

    return EParseResult::Parsed;
}

Parser::EParseResult Parser::ParseStructDefinition(Node** outNode)
{
    const Token structToken = mTokenParser->GetCurrentToken();
    
    if (structToken.mTokenString != "struct")
        return EParseResult::NotParsed;

    mTokenParser->Advance();
    const Token structNameToken = mTokenParser->GetCurrentToken();

    if (structNameToken.mTokenType != ETokenType::Identifier)
    {
        OnError("Invalid struct name: " + structNameToken.mTokenString);
        return EParseResult::Error;
    }

    // Create Node
    StructDefinition* structDefNode = new StructDefinition();
    structDefNode->mName = structNameToken.mTokenString;
    *outNode = structDefNode;

    mTokenParser->Advance();

    // Defined struct? (with body)
    if (mTokenParser->GetCurrentToken().mTokenString == "{")
    {
        mTokenParser->Advance();

        // Parse struct content
        Node** nextChildPtr = &structDefNode->mContent;
        while (mTokenParser->GetCurrentToken().mTokenString != "}")
        {
            Node* childNode = nullptr;
            EParseResult nodeParseResult = ParseNextNode(&childNode);
            if (nodeParseResult == EParseResult::Parsed)
            {
                *nextChildPtr = childNode;
                nextChildPtr = &(*nextChildPtr)->mNext;
            }
            else if (nodeParseResult == EParseResult::Error)
                return nodeParseResult;
            else
                break;
        }
        mTokenParser->Advance();
        if (mTokenParser->GetCurrentToken().mTokenString != ";")
        {
            // TODO: Delete node ???
            OnError("Missing ; after struct declaration.");
            return EParseResult::Error;
        }
        mTokenParser->Advance();

        return EParseResult::Parsed;
    }
    // Only struct declaration?
    else if (mTokenParser->GetCurrentToken().mTokenString == ";")
    {
        mTokenParser->Advance();
        return EParseResult::Parsed;
    }
    else
    {
        OnError("Expected { but found: " + mTokenParser->GetCurrentToken().mTokenString);
        return EParseResult::Error;
    }
}


Parser::EParseResult Parser::ParseNextNode(Node** outNode)
{
    const Token token = mTokenParser->GetCurrentToken();

    switch (token.mTokenType)
    {
    case ETokenType::Identifier:
    {
        // Struct  definition
        EParseResult parseResult = ParseStructDefinition(outNode);
        if (parseResult != EParseResult::NotParsed)
        {
            return parseResult;
        }

        // Function definition
        parseResult = ParseFunctionDefinition(outNode);
        if (parseResult != EParseResult::NotParsed)
        {
            return parseResult;
        }

        // Statement
        parseResult = ParseStatement(outNode);
        if (parseResult != EParseResult::NotParsed)
        {
            return parseResult;
        }

        // If we got here, we failed to parse a node
        OnError("Undefined identifier: " + token.mTokenString);
        return EParseResult::Error;

        break;
    }
    default:
    {
        OnError("Unexpected token: " + token.mTokenString);
        return EParseResult::Error;
        break;
    }
    }
}

/// FOR DEBUGGING
void PrintNodes(Node* node, int indents)
{
    std::string indentString = "";
    for (int i = 0; i < indents; i++)
        indentString += " ";

    Node* currNode = node;
    while (currNode != nullptr)
    {
        StructDefinition* structNode = dynamic_cast<StructDefinition*>(currNode);
        FunctionDefinition* funcNode = dynamic_cast<FunctionDefinition*>(currNode);
        VarDefStatement* varDefNode = dynamic_cast<VarDefStatement*>(currNode);

        if (structNode != nullptr)
        {
            LOG_INFO() << indentString << "Struct: " << structNode->mName;
            PrintNodes(structNode->mContent, indents+1);
        }
        else if (funcNode != nullptr)
        {
            LOG_INFO() << indentString << "Function: " << funcNode->mType << " " << funcNode->mName;
            PrintNodes(funcNode->mParams, indents + 1);
            PrintNodes(funcNode->mContent, indents + 1);
        }
        else if (varDefNode != nullptr)
        {
            LOG_INFO() << indentString << "Variable: " << varDefNode->mVariableType << " " << varDefNode->mVariableName;
        }

        currNode = node->mNext;
    }
}

void Parser::Parse()
{
    Block* rootNode = new Block();

    Node* currNode = rootNode;
    while (mTokenParser->HasMoreTokens())
    {
        Node* node;
        EParseResult result = ParseNextNode(&node);
        if (result == EParseResult::Parsed)
            currNode->mNext = node;
        else if (result == EParseResult::Error)
            return;

        PrintNodes(node, 0);
    }
}

void Parser::OnError(const std::string& errorString)
{
    LOG_ERROR() << errorString;
}
