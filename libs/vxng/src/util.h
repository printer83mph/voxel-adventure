#pragma once

#include <stdexcept>

class not_implemented_error : public std::logic_error {
  public:
    not_implemented_error() : std::logic_error("Not yet implemented.") {}

    virtual auto what() const noexcept -> char const * override {
        return "Not yet implemented.";
    }
};
