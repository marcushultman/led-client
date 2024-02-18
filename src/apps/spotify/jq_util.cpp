#include "jq_util.h"

namespace spotify {

bool nextBool(jq_state *jq, bool &value) {
  const auto jv = jq_next(jq);
  const auto kind = jv_get_kind(jv);
  if (kind == JV_KIND_TRUE) {
    value = true;
  } else if (kind == JV_KIND_FALSE || kind == JV_KIND_NULL) {
    value = false;
  } else {
    jv_free(jv);
    value = false;
    return false;
  }
  jv_free(jv);
  return true;
}

bool nextNumber(jq_state *jq, double &value) {
  const auto jv = jq_next(jq);
  const auto kind = jv_get_kind(jv);
  if (kind == JV_KIND_NUMBER) {
    value = jv_number_value(jv);
  } else if (kind == JV_KIND_NULL) {
    value = 0;
  } else {
    jv_free(jv);
    value = 0;
    return false;
  }
  jv_free(jv);
  return true;
}

bool nextStr(jq_state *jq, std::string &value) {
  const auto jv = jq_next(jq);
  const auto kind = jv_get_kind(jv);
  if (kind == JV_KIND_STRING) {
    value = jv_string_value(jv);
  } else if (kind == JV_KIND_NULL) {
    value.clear();
  } else {
    jv_free(jv);
    value.clear();
    return false;
  }
  jv_free(jv);
  return true;
}

}  // namespace spotify
