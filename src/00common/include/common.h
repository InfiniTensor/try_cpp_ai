﻿#ifndef COMMON_TYPES
#define COMMON_TYPES

#include "common/bf16_t.h"
#include "common/data_type.h"
#include "common/error_handler.h"
#include "common/fp16_t.h"
#include "common/range.h"
#include "common/rc.hpp"
#include "common/slice.h"
#include <absl/container/inlined_vector.h>
#include <memory>
#include <sstream>

namespace refactor {
    // 方便按“级别”定义整型数。

    using sint_lv0 = int8_t;
    using sint_lv1 = int16_t;
    using sint_lv2 = int32_t;
    using sint_lv3 = int64_t;

    using uint_lv0 = uint8_t;
    using uint_lv1 = uint16_t;
    using uint_lv2 = uint32_t;
    using uint_lv3 = uint64_t;

    using sint_min = sint_lv0;
    using sint_max = sint_lv3;
    using uint_min = uint_lv0;
    using uint_max = uint_lv3;

    template<class T> using Arc = std::shared_ptr<T>;

    template<typename T, size_t N> std::string vecToString(absl::InlinedVector<T, N> const &vec) {
        std::stringstream ss;
        ss << "[ ";
        for (auto d : vec) {
            ss << d << ' ';
        }
        ss << ']';
        return ss.str();
    }

}// namespace refactor

#endif// COMMON_TYPES
