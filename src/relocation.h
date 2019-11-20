#pragma once

#include <vector>
#include <string>

struct RelocationText
{
    std::vector<std::pair<size_t, std::string>> mSymAddrRefs; // TODO: refactor
    std::vector<size_t> mRelativeAddresses;
};
