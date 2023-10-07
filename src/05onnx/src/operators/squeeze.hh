﻿#ifndef ONNX_SQUEEZE_HH
#define ONNX_SQUEEZE_HH

#include "frontend/operator.h"

namespace refactor::onnx {
    using namespace frontend;

    struct Squeeze final : public Operator {

        Squeeze();

        static OpBox build(std::string_view, Attributes);
        static size_t typeId();

        size_t opTypeId() const final;
        std::string_view opTypeName() const final;
        InferResult infer(TensorRefs, InferOptions const &) const final;
        LowerOperator lower(TensorRefs) const final;
    };

}// namespace refactor::onnx

#endif// ONNX_SQUEEZE_HH
