#include "laf_algorithm_def.h"
#include "laf_algorithm_impl.h"

namespace fxlib {

namespace details {

std::shared_ptr<ilaf_impl> make_laf_impl(const std::string& type) {
  if (type == "112") {
    return std::shared_ptr<ilaf_impl>(new laf_alg<laf112_def>);
  } else if (type == "124") {
    return std::shared_ptr<ilaf_impl>(new laf_alg<laf124_def>);
  } else if (type == "212") {
    return std::shared_ptr<ilaf_impl>(new laf_alg<laf212_def>);
  }
  return std::shared_ptr<ilaf_impl>();
}

}  // namespace details

}  // namespace fxlib
