#pragma once

#include <string>

extern "C" {
#include <jq.h>
}

namespace spotify {

bool nextNumber(jq_state *jq, double &value);
bool nextStr(jq_state *jq, std::string &value);

}  // namespace spotify
