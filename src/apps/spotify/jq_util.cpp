#include "jq_util.h"

namespace spotify {

void nextStr(jq_state *jq, std::string &value) {
  const auto jv = jq_next(jq);
  if (jv_get_kind(jv) != JV_KIND_STRING) {
    jv_free(jv);
    value.clear();
    return;
  }
  value = jv_string_value(jv);
  jv_free(jv);
}

double nextNumber(jq_state *jq) {
  const auto jv = jq_next(jq);
  if (jv_get_kind(jv) != JV_KIND_NUMBER) {
    jv_free(jv);
    return 0;
  }
  const auto val = jv_number_value(jv);
  jv_free(jv);
  return val;
}

}  // namespace spotify
