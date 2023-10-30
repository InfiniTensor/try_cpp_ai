﻿#include "kernel/attributes/broadcaster.h"
#include <numeric>

namespace refactor::kernel {
    /// 多向广播的语义逻辑见 <https://github.com/onnx/onnx/blob/main/docs/Broadcasting.md#multidirectional-broadcasting>。
    ///
    /// 广播器在一次循环中构造各维度的步长，并执行 2 种优化：
    ///
    /// - 消除所有输入全为 1 的维度；
    /// - 尽量合并相邻的维度；
    ///   > 相邻维度可以合并的条件是在这些维度中，某个输入要么都发生了广播，要么都不需要广播。例如：
    ///   > {2, 3, 4, 5, 6} -> {6, 4, 30}
    ///   > {2, 3, 1, 5, 6}
    ///
    /// 为了实现这些优化，广播器维护和比较两个布尔向量，记录当前维度的广播状态是否变化。
    /// 所有输入在某个维度的步长会在这个维度确定下来时计算和保存。
    Broadcaster::Broadcaster(std::vector<Input> inputs) noexcept
        : strides{}, outputsCount(1), inputsCount(inputs.size()) {
        ASSERT(inputsCount > 0, "Broadcaster: no inputs");

        std::vector<bool> broadcastState(inputsCount, false);
        std::vector<uint_lv2> muls(inputsCount + 1, 1);
        while (true) {
            auto allEnd = true;
            uint_lv2 shape = 1;
            std::vector<bool> broadcastNext(inputsCount, false);
            for (auto i : range0_(inputsCount)) {
                if (inputs[i].it != inputs[i].end) {
                    allEnd = false;
                    if (auto dim = *inputs[i].it++; broadcastNext[i] = dim != 1) {
                        if (shape == 1) {
                            shape = dim;
                        } else {
                            ASSERT(shape == dim, "Broadcaster: shape mismatch");
                        }
                    }
                }
            }
            if (allEnd) { break; }
            if (shape == 1) { continue; }

            if (broadcastNext != broadcastState) {
                broadcastState = broadcastNext;
                strides.resize(strides.size() + inputsCount + 1, 0);

                auto itRev = strides.rbegin();
                for (auto i : range0_(inputsCount)) {
                    if (broadcastState[i]) {
                        *itRev = muls[i];
                        muls[i] *= shape;
                    }
                    ++itRev;
                }
                *itRev = muls[inputsCount];
            } else {
                for (auto i : range0_(inputsCount)) {
                    if (broadcastState[i]) {
                        muls[i] *= shape;
                    }
                }
            }
            muls[inputsCount] *= shape;
        }
        if (!strides.empty()) {
            std::reverse(strides.begin(), strides.end());
            outputsCount = muls[inputsCount] * strides.back();
        }
    }

    auto Broadcaster::build(TensorRefs const &inputs) -> std::vector<Input> {
        std::vector<Input> ans(inputs.size());
        std::transform(inputs.begin(), inputs.end(), ans.begin(),
                       [](auto const &t) { return Input{t.get().shape.rbegin(), t.get().shape.rend()}; });
        return ans;
    }
    auto Broadcaster::build(ShapeRefs const &inputs) -> std::vector<Input> {
        std::vector<Input> ans(inputs.size());
        std::transform(inputs.begin(), inputs.end(), ans.begin(),
                       [](auto const &s) { return Input{s.get().rbegin(), s.get().rend()}; });
        return ans;
    }

    Broadcaster::Broadcaster(TensorRefs const &inputs) noexcept
        : Broadcaster(build(inputs)) {}
    Broadcaster::Broadcaster(ShapeRefs const &inputs) noexcept
        : Broadcaster(build(inputs)) {}

    void Broadcaster::locate(uint_lv2 k, uint_lv2 ans[]) const noexcept {
        long rem = k;
        std::fill_n(ans, inputsCount, 0);
        for (auto i : range0_(strides.size() / (inputsCount + 1))) {
            auto dim = strides.data() + (inputsCount + 1) * i;
            auto div = std::div(rem, dim[inputsCount]);
            for (auto j : range0_(inputsCount)) { ans[j] += dim[j] * div.quot; }
            rem = div.rem;
        }
    }

}// namespace refactor::kernel
