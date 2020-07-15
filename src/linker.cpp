#include "linker.h"
#include <cstdio>
#include <fstream>
#include <cstring>

Linker::Linker(Emitter* emitter)
{
    mEmitter = emitter;
}

bool Linker::Link(const std::vector<CompilationUnit*> compUnits)
{
    // Collect symbols
    size_t currCUPos = 0xc000;
    for (CompilationUnit* compUnit : compUnits)
    {
        const size_t codeSize = compUnit->mObjectCode.size();
        
        // Collect symbols
        for (auto symPair : compUnit->mSymbolTable)
        {
            const ESymbolType symType = symPair.second->mSymbolType;
            if ((symType == ESymbolType::Function || symType == ESymbolType::Variable) && symPair.second->mAddrType != ESymAddrType::None)
            {
                if (mSymbolTable.find(symPair.first) != mSymbolTable.end())
                {
                    printf("ERROR: Symbol '%s' already defined.", symPair.first.c_str());
                    return false;
                }
                else
                {
                    if(symPair.second->mSymbolType == ESymbolType::Function)
                        symPair.second->mAddress += currCUPos; // variable symbols (data symbols) are not offset
                    mSymbolTable.insert(symPair);
                }
            }
        }

        // Relocate relative addresses
        for (const size_t codeAddr : compUnit->mRelocationText.mRelativeAddresses)
        {
            uint16_t* addrPtr = reinterpret_cast<uint16_t*>(&compUnit->mObjectCode[codeAddr]);
            *addrPtr += currCUPos;
        }

        currCUPos += codeSize;
    }

    if (mSymbolTable.find("_main") == mSymbolTable.end())
    {
        printf("ERROR: main not defined.");
        return false;
    }

    // Relocate symbol references
    for (CompilationUnit* compUnit : compUnits)
    {
        // Symbol references
        for (auto symRef : compUnit->mRelocationText.mSymAddrRefs)
        {
            const size_t codeAddr = symRef.first;
            auto symIter = mSymbolTable.find(symRef.second);
            if (symIter != mSymbolTable.end())
            {
                memcpy(&compUnit->mObjectCode[codeAddr], &symIter->second->mAddress, sizeof(uint16_t));
            }
            else
            {
                printf("LINKER ERROR: Undefined symbol: %s", symRef.second.c_str());
            }
        }
    }

    return WriteCode(compUnits);
}

bool Linker::WriteCode(const std::vector<CompilationUnit*> compUnits)
{
    char* data = mEmitter->GetData();
    // Write header
    data[0] = 'N';
    data[1] = 'E';
    data[2] = 'S';
    data[3] = 0x1a;
    data[4] = 0x01;
    data[5] = 0x01;
    data[6] = 0x01;
    data[7] = 0x00;
    data[8] = 0x00;
    data[9] = 0x00;
    data[10] = 0x00;
    data[11] = 0x00;
    data[12] = 0x00;
    data[13] = 0x00;
    data[14] = 0x00;
    data[15] = 0x00;

    // Write object code
    size_t currDataPos = 16;
    for (CompilationUnit* compUnit : compUnits)
    {
        const size_t dataSize = compUnit->mObjectCode.size();
        memcpy(&data[currDataPos], compUnit->mObjectCode.data(), dataSize);
        currDataPos += dataSize;
    }
    
    Symbol* mainSym = mSymbolTable["_main"];

    // Write entry point
    const size_t entryPoint = (currDataPos - 16) + 0xc000;
    mEmitter->SetWritePos(currDataPos);
    mEmitter->Emit("SEI");
    mEmitter->Emit("CLD");
    // Set stack pointer
    mEmitter->Emit("LDX", EAddressingMode::Immediate, 0xff);
    mEmitter->Emit("TXS");
    mEmitter->Emit("JMP", EAddressingMode::Absolute, mainSym->mAddress); // jump to main

    // Set reset vector
    *(int16_t*)&data[0xfffc - 0xc000 + 16] = entryPoint; // reset vector = entry point

    if (mEmitter->GetCurrentLocation() >= 0x10000)
    {
        printf("ERROR: ROM size exceeded. Wrote %i bytes.", mEmitter->GetCurrentLocation());
        return false;
    }
    else
        return true;
}

void Linker::WriteROM()
{
    std::ofstream romStream("testrom.nes", std::ios::out | std::ios::binary);
    
    char* data = mEmitter->GetData();

    romStream.write(data, 0x10000);
    romStream.close();
}
