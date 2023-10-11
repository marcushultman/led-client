#pragma once

#include <string>

extern "C" {
#include <jq.h>
}

namespace spotify {

void nextStr(jq_state *jq, std::string &value);

double nextNumber(jq_state *jq);

}  // namespace spotify
