#pragma once
#include <utility>
#include "ErrorCode.h"

template<typename T>
struct Result {
    T value;
    ErrorCode error = ErrorCode::Success;

    bool has_value() const { return error == ErrorCode::Success; }
    explicit operator bool() const { return has_value(); }
    T& operator*() { return value; }
    T* operator->() { return &value; }

    static Result<T> Success(T val) { return {std::move(val), ErrorCode::Success}; }
    static Result<T> Error(ErrorCode code) { return {T{}, code}; }
};
