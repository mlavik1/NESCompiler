#include "tokeniser.h"
#include "debug.h"
#include <fstream>

#include "tokeniser.h"
#include "parser.h"

int main(int args, char** argv)
{
    LOG_INFO() << "starting";

    std::string filePath = "../tests/test1.c";
    std::ifstream fileStream(filePath);
    std::string fileContents((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

    CompilationUnit compilationUnit;

    TokenParser tokenParser(fileContents.c_str());
    
    Parser parser(&tokenParser, &compilationUnit);
    parser.Parse();

    return 0;
}