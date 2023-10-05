#pragma once

#include <iostream>
#include <string>
#include <unordered_set>

namespace storage {

void loadSet(std::unordered_set<std::string> &, std::istream &&);
void saveSet(const std::unordered_set<std::string> &, std::ostream &);

}  // namespace storage
