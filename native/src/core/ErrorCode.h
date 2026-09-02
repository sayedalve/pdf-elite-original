#pragma once

enum class ErrorCode {
    Success = 0,
    UnknownError,
    FileOpenFailed,
    FileTooLarge,
    AccessDenied,
    InvalidFormat,
    PasswordRequired,
    InvalidPassword,
    PageNotFound,
    RenderFailed,
    MemoryAllocationFailed,
    NotInitialized
};
