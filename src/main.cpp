#include "tokeniser.h"
#include <fstream>

#include "tokeniser.h"
#include "parser.h"
#include "analyser.h"
#include "code_generator.h"
#include "emitter.h"
#include "opcode.h"
#include "linker.h"
#include <vector>
#include <cstring>

int main(int args, char** argv)
{
    std::vector<std::string> inputFiles;
    std::string outputFile = "";
    enum EArgParseMode { Input, Output } argParseMode = EArgParseMode::Input;

    for (int i = 1; i < args; ++i)
    {
        if (argParseMode == EArgParseMode::Input)
        {
            if (strcmp(argv[i], "-o") == 0)
                argParseMode = EArgParseMode::Output;
            else
                inputFiles.push_back(argv[i]);
        }
        else
            outputFile = argv[i];
    }
    if (outputFile == "")
    {
        printf("No output file.\n");
        return 0;
    }
    else if (inputFiles.size() == 0)
    {
        printf("No input files.\n");
        return 0;
    }

    OpcodeTranslator* opcodeTranslator = new OpcodeTranslator();
    DataAllocator* dataAllocator = new DataAllocator();

    std::vector<CompilationUnit*> compilationUnits;

    for (size_t iSrc = 0; iSrc < inputFiles.size(); ++iSrc)
    {
        std::string filePath = inputFiles[iSrc];
        std::ifstream fileStream(filePath);
        std::string fileContents((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

        CompilationUnit* compUnit = new CompilationUnit();

        // Tokenise
        TokenParser tokenParser(fileContents.c_str());

        // Parse
        Parser parser(&tokenParser, compUnit);
        parser.Parse();

        // Analyse
        Analyser analyser(compUnit);
        analyser.Analyse();

        compilationUnits.push_back(compUnit);

        // Compile
        Emitter emitter(opcodeTranslator);
        CodeGenerator generator(compUnit, &emitter, dataAllocator);
        generator.Generate();

        size_t dataSize = emitter.GetDataSize();
        compUnit->mObjectCode.resize(dataSize); // TODO
        memcpy(compUnit->mObjectCode.data(), emitter.GetData(), dataSize);
    }

    // Link
    Emitter emitter(opcodeTranslator);
    Linker linker(&emitter);

    if (linker.Link(compilationUnits))
    {
        linker.WriteROM();
    }

    getchar();

    return 0;
}
