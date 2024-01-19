#ifndef KERNEL_PAD_ATTRIBUTES_H
#define KERNEL_PAD_ATTRIBUTES_H

#include "../tensor.h"
#include "common.h"

namespace refactor::kernel {

    struct PadType {
        enum : uint8_t {
            Constant,
            Reflect,
            Edge,
            Wrap,
        } type;

        constexpr PadType() noexcept
            : type(Constant) {}
        constexpr PadType(decltype(type) type_) noexcept
            : type(type_) {}
        constexpr operator decltype(type)() const noexcept {
            return type;
        }
        constexpr std::string_view toString() const noexcept {
            switch (type) {
                case Constant:
                    return "Constant";
                case Reflect:
                    return "Reflect";
                case Edge:
                    return "Edge";
                case Wrap:
                    return "Wrap";
                default:
                    UNREACHABLE();
            }
        }
    };

    using PadsShape = absl::InlinedVector<int64_t, 4>;


    struct PadInfo {
        int rank;
        PadType mode;
        PadsShape pads;
        PadsShape wholeNDim;
        PadsShape partNDim;
        PadsShape partStride;
        DataType type;
        bool have_value;
        size_t size;

        explicit PadInfo(PadsShape, PadType, Tensor const &, Tensor const &, bool) noexcept;
    };


}// namespace refactor::kernel

#endif// KERNEL_PAD_ATTRIBUTES_H
