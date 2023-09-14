﻿#include "infer.h"

namespace refactor::onnx {
    using namespace refactor::common;

    enum class Ty {
        Add,
        Sub,
        Mul,
        Div
    };

    template<DataType T>
    void calculate(Ty ty, void *dst, void *a, void *b) {
        using T_ = typename primitive_t<T>::type;
        auto a_ = *reinterpret_cast<T_ *>(a);
        auto b_ = *reinterpret_cast<T_ *>(b);
        auto dst_ = reinterpret_cast<T_ *>(dst);
        switch (ty) {
            case Ty::Add:
                *dst_ = a_ + b_;
                break;
            case Ty::Sub:
                *dst_ = a_ - b_;
                break;
            case Ty::Mul:
                *dst_ = a_ * b_;
                break;
            case Ty::Div:
                *dst_ = a_ / b_;
                break;
            default:
                RUNTIME_ERROR("Unreachable");
        }
        fmt::print("{} ", *dst_);
    }

    InferResult inferArithmetic(Operator const &op, Tensors inputs) {
        EXPECT_SIZE(2) {
            auto const &a = inputs[0];
            auto const &b = inputs[1];
            auto dataType = a->dataType;
            if (!isNumbericDataType(dataType) || b->dataType != dataType) {
                return Err(InferError(ERROR_MSG("Data type not support")));
            }

            auto res = multidirBroadcast({a->shape, b->shape});
            if (res.isErr()) {
                return Err(InferError(ERROR_MSG(res.unwrapErr())));
            }
            auto ans = Tensor::share(dataType, std::move(res.unwrap()));
            if (!shouldCalculate(inputs, ans->shape)) {
                return Ok(Tensors{std::move(ans)});
            }

            auto size = ans->elementsSize();
            auto eleSize = dataTypeSize(dataType);
            auto dst = reinterpret_cast<uint8_t *>(ans->malloc());
            fmt::print("( {} dst<{}> = ", op.opType.name(), size);
            for (size_t i = 0; i < size; ++i) {
                auto ty = op.opType.is("onnx::Add")   ? Ty::Add
                          : op.opType.is("onnx::Sub") ? Ty::Sub
                          : op.opType.is("onnx::Mul") ? Ty::Mul
                          : op.opType.is("onnx::Div") ? Ty::Div
                                                      : UNREACHABLE();
                auto indices = locateN(ans->shape, i);
                auto a_ = locate1(*a, indices),
                     b_ = locate1(*b, indices);
                auto dst_ = dst + i * eleSize;
                //-------------------------------------
#define CASE(T)                                   \
    case DataType::T:                             \
        calculate<DataType::T>(ty, dst_, a_, b_); \
        break
                //-------------------------------------
                switch (dataType) {
                    CASE(F32);
                    CASE(F64);
                    CASE(I32);
                    CASE(I64);
                    CASE(I8);
                    CASE(I16);
                    CASE(U8);
                    CASE(U16);
                    CASE(U32);
                    CASE(U64);
                    default:
                        ans->free();
                        break;
                }
            }
            fmt::print(")");
            return Ok(Tensors{std::move(ans)});
        }
    }
}// namespace refactor::onnx
