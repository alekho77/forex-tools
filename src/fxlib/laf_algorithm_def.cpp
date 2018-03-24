#include "laf_algorithm_def.h"
#include "laf_algorithm_impl.h"

namespace fxlib {

namespace details {

std::shared_ptr<ilaf_impl> make_laf_impl(const std::string& type) {
    if (type == "112") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf1xx_def<12>>);
    } else if (type == "124") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf1xx_def<24>>);
    } else if (type == "212") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf2xx_def<12>>);
    } else if (type == "212b") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf2xxb_def<12>>);
    } else if (type == "224") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf2xx_def<24>>);
    } else if (type == "148") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf1xx_def<48>>);
    } else if (type == "248") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf2xx_def<48>>);
    } else if (type == "312") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf3xx_def<12>>);
    } else if (type == "324") {
        return std::shared_ptr<ilaf_impl>(new laf_alg<laf3xx_def<24>>);
    } else if (type == "348") {
        return std::shared_ptr<ilaf_impl>(
            new laf_alg<laf3xx_def<48>>);  // TODO: resolve problem of Compiler Warning (level 1) C4503
    }
    return std::shared_ptr<ilaf_impl>();
}

}  // namespace details

}  // namespace fxlib
