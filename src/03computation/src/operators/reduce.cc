﻿#include "computation/operators/reduce.h"

namespace refactor::computation {

    size_t Reduce::typeId(ReduceType type) {
        switch (type) {
            case ReduceType::Mean: {
                static uint8_t ID = 1;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::L1: {
                static uint8_t ID = 2;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::L2: {
                static uint8_t ID = 3;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::LogSum: {
                static uint8_t ID = 4;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::LogSumExp: {
                static uint8_t ID = 5;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::Max: {
                static uint8_t ID = 6;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::Min: {
                static uint8_t ID = 7;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::Prod: {
                static uint8_t ID = 8;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::Sum: {
                static uint8_t ID = 9;
                return reinterpret_cast<size_t>(&ID);
            }
            case ReduceType::SumSquare: {
                static uint8_t ID = 10;
                return reinterpret_cast<size_t>(&ID);
            }
            default:
                UNREACHABLE();
        }
    }
    size_t Reduce::opTypeId() const {
        return typeId(type);
    }
    std::string_view Reduce::name() const {
        switch (type) {
            case ReduceType::Mean:
                return "ReduceMean";
            case ReduceType::L1:
                return "ReduceL1";
            case ReduceType::L2:
                return "ReduceL2";
            case ReduceType::LogSum:
                return "ReduceLogSum";
            case ReduceType::LogSumExp:
                return "ReduceLogSumExp";
            case ReduceType::Max:
                return "ReduceMax";
            case ReduceType::Min:
                return "ReduceMin";
            case ReduceType::Prod:
                return "ReduceProd";
            case ReduceType::Sum:
                return "ReduceSum";
            case ReduceType::SumSquare:
                return "ReduceSumSquare";
            default:
                UNREACHABLE();
        }
    }

}// namespace refactor::computation
