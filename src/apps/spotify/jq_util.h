#pragma once

#include <string>

extern "C" {
#include <jq.h>
}

namespace spotify {

bool nextBool(jq_state *jq, bool &value);
bool nextNumber(jq_state *jq, double &value);
bool nextStr(jq_state *jq, std::string &value);

}  // namespace spotify
