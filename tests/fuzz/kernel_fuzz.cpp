include<fuzztest / fuzztest.h>
#include <algorithm>
#include <gtest/gtest.h>
#include <string>

    bool IsValidIdentifier(const std::string &id, int limit) {
  if (id.empty() || limit <= 0)
    return false;

  // Check if string is too long for the buffer limit
  if (id.length() > static_cast<size_t>(limit))
    return false;

  // Only allow alphanumeric characters
  return std::all_of(id.begin(), id.end(),
                     [](char c) { return std::isalnum(c); });
}

void FuzzBasicValidation(const std::string &random_str, int random_limit) {
  // We call our basic function with the fuzzer's "garbage" data
  bool result = IsValidIdentifier(random_str, random_limit);

  // PROPERTIES (Invariants):
  // If the function returns true, these things MUST be true.
  // If the fuzzer breaks these, it found a bug!
  if (result) {
    EXPECT_FALSE(random_str.empty());
    EXPECT_LE(random_str.length(), static_cast<size_t>(random_limit));
  }
}

FUZZ_TEST(BasicSuite, FuzzBasicValidation)
    .WithDomains(
        fuzztest::String(), // Try every possible string (nulls, emojis, etc)
        fuzztest::InRange(-10, 100) // Try numbers, including negatives/zero
    );
