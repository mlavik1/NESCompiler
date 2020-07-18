#pragma once

#include <string>
#include "tokeniser.h"

enum class PreprocessorScopeType
{
   IfBody,
   ElseBody
};

struct PreprocessorScope
{
   PreprocessorScopeType mScopeType;
   bool mIgnoreContent = false;
};

enum class PreprocessorDirective
{
   Define,
   Ifdef,
   Ifndef,
   Else,
   Endif,
   Include,
   Invalid
};

class Preprocessor
{
private:
   TokenParser& mTokenParser;
   std::string mFileDir;
   std::stack<PreprocessorScope> mScopeStack;
   std::unordered_map<std::string, std::string> mDefinitions;
   std::vector<Token> mPreprocessedTokens;

   PreprocessorDirective GetPreprocessorDirective(const std::string& inToken);
   void ProcessToken(Token inToken);
   bool IsCurrentScopeIgnored();

public:
   Preprocessor(TokenParser& inTokenParser, std::string fileDir);

   void AddDefinition(const std::string name, const std::string value);
   void Preprocess();
};
