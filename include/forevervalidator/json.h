#ifndef FOREVERVALIDATOR_JSON_H
#define FOREVERVALIDATOR_JSON_H

#include <string>
#include <forevervalidator/validation.h>

namespace forevervalidator {
Result<std::string> SerializeValidationReport(
        const ValidationReport &report) noexcept;
Result<std::string> SerializeValidationError(
        const ValidationError &error) noexcept;
}  // namespace forevervalidator

#endif
