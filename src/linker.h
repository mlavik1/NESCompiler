#pragma once

#include "compilation_unit.h"
#include "emitter.h"
#include <vector>

class Linker
{
private:
    std::unordered_map<std::string, Symbol*> mSymbolTable;
    Emitter* mEmitter;
    bool WriteCode(const std::vector<CompilationUnit*> compUnits);

public:
    Linker(Emitter* emitter);
    bool Link(const std::vector<CompilationUnit*> compUnits);
    void WriteROM();
};
