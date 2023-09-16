﻿#include "computation/graph.h"
#include "common/error_handler.h"
#include "computation/tensor.h"
#include <chrono>
#include <fmtlog.h>

using namespace refactor::common;
using namespace std::chrono;

namespace refactor::computation {

    Graph::Graph(graph_topo::Graph<Node, Edge> internal)
        : _internal(std::move(internal)), _variables() {
        collectVariables();
    }

    void Graph::collectVariables() {
        for (auto &edge : _internal.edges) {
            if (edge.tensor) {
                for (auto &dim : edge.tensor->shape) {
                    if (dim.isVariable()) {
                        auto const &var = dim.variable();
                        auto [it, ok] = _variables.try_emplace(var->name, var);
                        if (!ok) {// varibales with same name is same variable
                            dim = DimExpr(it->second);
                        }
                    }
                }
            }
        }
    }

    auto Graph::internal() const -> decltype(_internal) const & {
        return _internal;
    }

    bool Graph::substitute(const char *name, int64_t value) {
        if (auto it = _variables.find(name); it != _variables.end()) {
            it->second->value = value;
            return true;
        } else {
            return false;
        }
    }

    bool Graph::setInput(size_t i, std::shared_ptr<Tensor> tensor) {
        if (i >= _internal.topology.globalInputsCount()) { return false; }
        auto current = _internal.edges[i];
        if (!current.tensor) {
            current.tensor = std::move(tensor);
            return true;
        }
        auto &shape0 = current.tensor->shape;
        auto const &shape1 = tensor->shape;
        auto rank = shape0.size();
        if (shape1.size() != rank) { return false; }
        for (size_t j = 0; j < rank; ++j) {
            if (shape0[j].isVariable()) {
                if (shape1[j].isVariable() && shape0[j].variable()->name != shape1[j].variable()->name) {
                    return false;
                }
                if (shape1[j].hasValue()) {
                    shape0[j].variable()->value = shape1[j].value();
                }
            } else if (shape1[j].isVariable() || shape0[j].value() != shape1[j].value()) {
                return false;
            }
        }
        current.tensor->dataType = tensor->dataType;
        current.tensor->data = std::move(tensor->data);
        return true;
    }

    class LogGuard {
    public:
        ~LogGuard() {
            fmtlog::poll();
        }
    };

    std::unordered_set<std::string> Graph::fillEdgeInfo() {
        std::unordered_set<std::string> unknownVariables;// 未知变量，将返回。
        LogGuard _logGuard;
        logi("edge inference start");
        auto const startTime = steady_clock::now();
        // 拓扑遍历
        for (auto [nodeIdx, inputs, outputs] : _internal.topology) {
            // 构造入边
            std::optional<std::vector<std::shared_ptr<Tensor>>> inputs_(std::in_place);

            inputs_->reserve(inputs.size());
            for (auto i : inputs) {
                if (!_internal.edges[i].tensor) {
                    // 无入边，跳过节点
                    inputs_ = std::nullopt;
                    break;
                }
                auto const &input = _internal.edges[i].tensor;
                ASSERT(input, "input edge not exist");
                inputs_->emplace_back(input);
            }
            if (!inputs_) { continue; }

            auto const &node = _internal.nodes[nodeIdx];
            auto msg = fmt::format("nodes[{}] = {}({})", nodeIdx, node.name, node.op->opType.name());
            // 推导
            auto infered = node.op->infer(std::move(*inputs_));
            if (infered.isErr()) {
                msg += ", inference failed";
                // 推导失败，记录未知变量
                auto error = infered.unwrapErr();
                if (std::holds_alternative<UnknownVariable>(error.value)) {
                    unknownVariables.insert(std::get<UnknownVariable>(error.value).name);
                } else {
                    throw error;
                }
            } else {
                // 推导成功，填充边信息
                auto infered_ = infered.unwrap();
                if (infered_.size() < outputs.size()) {
                    OUT_OF_RANGE("outputs more than infered", infered_.size(), outputs.size());
                } else {
                    msg += ", outputs = ( ";
                    for (auto const &tensor : infered_) {
                        msg += shapeFormat(tensor->shape);
                    }
                    msg += " )";
                    for (auto i : range0_(outputs.size())) {
                        _internal.edges[outputs[i]].tensor = std::move(infered_[i]);
                    }
                }
            }
            logi("{}", std::move(msg));
        }
        auto const endTime = steady_clock::now();
        logi("inference cost time: {}μs", duration_cast<microseconds>(endTime - startTime).count());
        if (unknownVariables.empty()) {
            std::unordered_set<std::string> frontNodes, dynamicNodes;
            auto it = _internal.topology.begin();
            auto const end = _internal.topology.end();
            {
                logi("compute on device: ");
                auto i = 0;
                while (it != end) {
                    auto [nodeIdx, inputs, outputs] = *it++;
                    if (std::any_of(outputs.begin(), outputs.end(), [&](auto i) { return !_internal.edges[i].tensor->hasData(); })) {
                        auto node = _internal.nodes[nodeIdx];
                        logi("{:>8}. {}", i++, node.name);
                        dynamicNodes.insert(std::string(node.op->opType.name()));
                        if (std::all_of(inputs.begin(), inputs.end(), [&](auto i) { return _internal.edges[i].tensor->hasData(); })) {
                            frontNodes.insert(std::string(node.op->opType.name()));
                        }
                    }
                }
            }
            {
                logi("types:");
                auto i = 0;
                for (auto const &node : dynamicNodes) {
                    if (frontNodes.erase(node)) {
                        logi("{:>8}.*{}", i++, node);
                    } else {
                        logi("{:>8}. {}", i++, node);
                    }
                }
            }
            {
                logi("outputs:");
                auto i = 0;
                for (auto edgeIdx : it.globalOutputs()) {
                    auto const &edge = _internal.edges[edgeIdx];
                    logi("    outputs[{:>2}] = {} with {}", i++, edge.name, shapeFormat(edge.tensor->shape));
                }
            }
        }
        return unknownVariables;
    }

}// namespace refactor::computation
