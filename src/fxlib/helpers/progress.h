#pragma once

#include <iterator>
#include <ostream>

namespace fxlib {
namespace helpers {
class progress {
 public:
    progress(size_t size, std::ostream& out) : out_(out) {
        for (size_t i = 0; i < std::size(progress_idxs_); i++) {
            progress_idxs_[i] = ((i + 1) * size) / std::size(progress_idxs_);
        }
    }
    void operator()(size_t curr) {
        while (curr_idx_ < std::size(progress_idxs_) && progress_idxs_[curr_idx_] <= (curr + 1)) {
            out_ << ".";
            ++curr_idx_;
            if (curr_idx_ % 10 == 0) {
                out_ << " " << curr_idx_ << "%" << std::endl;
            }
        }
    }

 private:
    std::ostream& out_;
    size_t progress_idxs_[100];
    size_t curr_idx_ = 0;
};
}  // namespace helpers
}  // namespace fxlib
