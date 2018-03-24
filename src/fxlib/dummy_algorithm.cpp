#include "dummy_algorithm.h"
#include "helpers/string_conversion.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

namespace {
ForecastInfo from_cfg(const boost::property_tree::ptree& settings) {
    ForecastInfo info{};
    info.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
    info.window = conversion::duration_from_string(settings.get<std::string>("window"));
    info.timeout = conversion::duration_from_string(settings.get<std::string>("timeout"));
    info.margin = settings.get<double>("margin");
    return info;
}
}  // namespace

DummyAlgorithm::DummyAlgorithm(const boost::property_tree::ptree& settings)
    : info_(from_cfg(settings)), gen_(settings.get("seed", std::random_device()())), dis_() {}

double DummyAlgorithm::Feed(const fxcandle&) {
    return dis_(gen_);
}

void DummyAlgorithm::Reset() {
    gen_ = std::mt19937(std::random_device()());
}

}  // namespace fxlib
