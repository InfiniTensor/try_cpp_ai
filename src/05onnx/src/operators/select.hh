﻿#ifndef ONNX_SELECT_HH
#define ONNX_SELECT_HH

#include "frontend/operator.h"

namespace refactor::onnx {
    using namespace frontend;

    enum class SelectType {
        Max,
        Min,
    };

    struct Select final : public Operator {
        SelectType type;

        explicit Select(SelectType);

        static OpBox build(std::string_view, Attributes);
        static size_t typeId(SelectType);

        size_t opTypeId() const final;
        std::string_view opTypeName() const final;
        InferResult infer(TensorRefs, InferOptions const &) const final;
        LowerOperator lower(TensorRefs) const final;
    };

}// namespace refactor::onnx

#endif// ONNX_SELECT_HH
