#pragma once
#include <exception>

class TimeLimitExceededException : public std::exception {
    public:
    const char* what() const noexcept override {
        return "Time limit for search was exceeded.";
    }
};