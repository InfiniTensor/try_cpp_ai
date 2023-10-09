﻿#include "kernel/collectors/simple_binary.h"
#include "../kernels/simple_binary/arthimetic11.hh"
#include "../kernels/simple_binary/arthimetic11_cuda.hh"
#include "common/error_handler.h"

namespace refactor::kernel {

#define REGISTER(T)                                \
    if (auto box = T::build(type, a, b, c); box) { \
        ans.emplace_back(std::move(box));          \
    }

    std::vector<KernelBox>
    SimpleBinaryCollector::filter(TensorRefs inputs, TensorRefs outputs) const {
        auto const &a = inputs[0].get();
        auto const &b = inputs[1].get();
        auto const &c = outputs[0].get();

        std::vector<KernelBox> ans;
        switch (target) {
            case Target::Cpu:
                REGISTER(Arthimetic11)
                break;
            case Target::NvidiaGpu:
                REGISTER(Arthimetic11Cuda)
                break;
            default:
                UNREACHABLEX(void, "Unknown target");
        }
        return ans;
    }

}// namespace refactor::kernel
