﻿#include "common/data_type.h"
#include "common/error_handler.h"
#include <fmt/core.h>
#include <unordered_set>

namespace refactor::common {
    std::optional<DataType> parseDataType(uint8_t value) {
        switch (value) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
                return {(DataType) value};
            default:
                return {};
        }
    }

    std::string_view dataTypeName(DataType dt) {
        switch (dt) {
            case DataType::F32:
                return "F32";
            case DataType::U8:
                return "U8 ";
            case DataType::I8:
                return "I8 ";
            case DataType::U16:
                return "U16";
            case DataType::I16:
                return "I16";
            case DataType::I32:
                return "I32";
            case DataType::I64:
                return "I64";
            case DataType::Bool:
                return "Bool";
            case DataType::FP16:
                return "FP16";
            case DataType::F64:
                return "F64";
            case DataType::U32:
                return "U32";
            case DataType::U64:
                return "U64";
            case DataType::Complex64:
                return "Complex64";
            case DataType::Complex128:
                return "Complex128";
            case DataType::BF16:
                return "BF16";
            default:
                RUNTIME_ERROR("Unreachable");
        }
    }

    bool isIeee754DataType(DataType dt) {
        static const std::unordered_set<DataType> set{DataType::F32, DataType::FP16, DataType::F64};
        return set.find(dt) != set.end();
    }

    bool isFloatDataType(DataType dt) {
        static const std::unordered_set<DataType> set{DataType::F32, DataType::FP16, DataType::F64, DataType::BF16};
        return set.find(dt) != set.end();
    }

    bool isSignedDataType(DataType dt) {
        static const std::unordered_set<DataType> set{
            DataType::F32, DataType::I32, DataType::I64, DataType::FP16, DataType::F64, DataType::BF16};
        return set.find(dt) != set.end();
    }

    bool isNumbericDataType(DataType dt) {
        static const std::unordered_set<DataType> set{
            DataType::F32, DataType::U8, DataType::I8, DataType::U16, DataType::I16, DataType::I32,
            DataType::I64, DataType::FP16, DataType::F64, DataType::U32, DataType::U64, DataType::BF16};
        return set.find(dt) != set.end();
    }

    bool isBool(DataType dt) {
        return dt == DataType::Bool;
    }

    size_t dataTypeSize(DataType dt) {
#define RETURN_SIZE(TYPE) \
    case DataType::TYPE:  \
        return sizeof(primitive_t<DataType::TYPE>::type)

        switch (dt) {
            RETURN_SIZE(F32);
            RETURN_SIZE(U8);
            RETURN_SIZE(I8);
            RETURN_SIZE(U16);
            RETURN_SIZE(I16);
            RETURN_SIZE(I32);
            RETURN_SIZE(I64);
            RETURN_SIZE(Bool);
            RETURN_SIZE(FP16);
            RETURN_SIZE(F64);
            RETURN_SIZE(U32);
            RETURN_SIZE(U64);
            RETURN_SIZE(Complex64);
            RETURN_SIZE(Complex128);
            RETURN_SIZE(BF16);
            default:
                RUNTIME_ERROR("Unreachable");
        }
    }

}// namespace refactor::common
