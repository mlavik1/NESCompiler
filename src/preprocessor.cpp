#include "preprocessor.h"
#include <fstream>

Preprocessor::Preprocessor(TokenParser& inTokenParser, std::string fileDir)
   : mTokenParser(inTokenParser), mFileDir(fileDir)
{

}

bool Preprocessor::IsCurrentScopeIgnored()
{
   return !mScopeStack.empty() && mScopeStack.top().mIgnoreContent;
}

PreprocessorDirective Preprocessor::GetPreprocessorDirective(const std::string& inToken)
{
   if (inToken == "#define")
   {
      return PreprocessorDirective::Define;
   }
   else if (inToken == "#ifdef")
   {
      return PreprocessorDirective::Ifdef;
   }
   else  if (inToken == "#ifndef")
   {
      return PreprocessorDirective::Ifndef;
   }
   else if (inToken == "#else")
   {
      return PreprocessorDirective::Else;
   }
   else if (inToken == "#endif")
   {
      return PreprocessorDirective::Endif;
   }
   else if (inToken == "#include")
   {
      return PreprocessorDirective::Include;
   }
   else
      return PreprocessorDirective::Invalid;
}

void Preprocessor::ProcessToken(Token inToken)
{
   PreprocessorDirective directive = PreprocessorDirective::Invalid;
   if (inToken.mTokenType == ETokenType::PreprocessorDirective)
      directive = GetPreprocessorDirective(inToken.mTokenString);

   // handle preprocessor directives
   if (directive != PreprocessorDirective::Invalid)
   {
      switch (directive)
      {
      case PreprocessorDirective::Define:
      {
         if (!IsCurrentScopeIgnored())
         {
            mTokenParser.Advance();
            const Token defNameToken = mTokenParser.GetCurrentToken();
            mTokenParser.Advance();
            std::string defName = defNameToken.mTokenString;
            const Token defValToken = mTokenParser.GetCurrentToken();
            AddDefinition(defName, defValToken);
         }
         break;
      }
      case PreprocessorDirective::Ifdef:
      case PreprocessorDirective::Ifndef:
      {
         mTokenParser.Advance();
         std::string def = mTokenParser.GetCurrentToken().mTokenString;
         PreprocessorScope scope;
         scope.mScopeType = PreprocessorScopeType::IfBody;
         scope.mIgnoreContent = IsCurrentScopeIgnored() || (mDefinitions.find(def) == mDefinitions.end()) == (directive == PreprocessorDirective::Ifdef);
         mScopeStack.push(scope);
         break;
      }
      case PreprocessorDirective::Else:
      {
         PreprocessorScope scope = mScopeStack.top();
         mScopeStack.pop();
         scope.mScopeType = PreprocessorScopeType::ElseBody;
         scope.mIgnoreContent = IsCurrentScopeIgnored() || !scope.mIgnoreContent;
         mScopeStack.push(scope);
         break;
      }
      case PreprocessorDirective::Endif:
      {
         mScopeStack.pop();
         break;
      }
      case PreprocessorDirective::Include:
      {
         mTokenParser.Advance();
         // Read included file
         std::string includePath = mTokenParser.GetCurrentToken().mTokenString;
         includePath = includePath.substr(1, includePath.size() - 2);
         includePath = mFileDir + "/" + includePath;
         std::ifstream includedFile(includePath);
         std::string fileContent((std::istreambuf_iterator<char>(includedFile)), std::istreambuf_iterator<char>());

         // Parse included file
         std::vector<Token> newTokens;
         Tokeniser tokeniser(fileContent.c_str());
         while (true)
         {
            Token token = tokeniser.ParseToken();
            if (token.mTokenString != "")
            {
               newTokens.push_back(token);
            }
            else if (token.mTokenType == ETokenType::EndOfFile)
            {
               break;
            }
         }

         // Copy new tokens
         std::vector<Token>& tokens = mTokenParser.GetTokens();
         size_t numTokens = tokens.size();
         size_t nextTokenIndex = mTokenParser.GetCurrentTokenIndex() + 1;
         tokens.insert(tokens.begin() + nextTokenIndex, newTokens.begin(), newTokens.end());
         break;
      }
      }
   }
   else if (!IsCurrentScopeIgnored())
   {
      // replace preprocessor definition
      if (inToken.mTokenType == ETokenType::Identifier)
      {
         auto defIter = mDefinitions.find(inToken.mTokenString);
         if (defIter != mDefinitions.end())
         {
            inToken = defIter->second;
         }
      }
      // push preprocessed token
      if (inToken.mTokenType != ETokenType::NewLine)
         mPreprocessedTokens.push_back(inToken);
   }
}

void Preprocessor::AddDefinition(std::string name, Token token)
{
   mDefinitions.emplace(name, token);
}

void Preprocessor::Preprocess()
{
   while (mTokenParser.HasMoreTokens())
   {
      Token token = mTokenParser.GetCurrentToken();

      ProcessToken(token);

      mTokenParser.Advance();
   }
   mTokenParser.SetTokens(mPreprocessedTokens);
   mTokenParser.ResetPosition();
}
