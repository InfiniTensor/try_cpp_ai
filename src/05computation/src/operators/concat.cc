﻿#include "computation/operators/concat.h"
#include "kernel/collectors/concat.h"

namespace refactor::computation {
    using Op = Concat;

    auto Op::typeId() noexcept -> size_t {
        static uint8_t ID = 1;
        return reinterpret_cast<size_t>(&ID);
    }
    auto Op::opTypeId() const noexcept -> size_t { return typeId(); }
    auto Op::name() const noexcept -> std::string_view { return "Concat"; }
    auto Op::candidateKernels(Target target) const noexcept -> kernel::CollectorBox {
        using Collector_ = kernel::ConcatCollector;
        return std::make_unique<Collector_>(target, axis);
    }

    Shape ConcatBox::verify(Tensor const &a, Tensor const &b) const noexcept {
        Shape ans = {};
        if (a.rank() != 2 || b.rank() != 2) {
            return ans;
        }
        if (a.shape[0] != b.shape[0]) {
            return ans;
        }
        if (a.dataType != b.dataType) {
            return ans;
        }
        ans = {a.shape[0], a.shape[1] + b.shape[1]};
        return ans;
    }
    bool ConcatBox::compute(Tensor const &a, Tensor const &b, Tensor &out) const noexcept {

        if (a.data == nullptr || b.data == nullptr) {
            return false;
        }
        //compute
        auto kernels = this->base->candidateKernels(Target::Cpu)->filter({a, b}, {out});
        ASSERT(kernels.size() != 0, "do not supposrt this kernel");
        runtime::Resources res;
        auto rou = kernels[0]->lower(res);
        void const *inputs[]{*a.data, *b.data};
        void *outputs[]{*out.data};
        rou(res, inputs, outputs);
        return true;
    }

}// namespace refactor::computation
