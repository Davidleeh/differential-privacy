//
// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "algorithms/util.h"

#include "base/testing/status_matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "algorithms/distributions.h"
#include "algorithms/numerical-mechanisms-testing.h"

namespace differential_privacy {
namespace {

using ::testing::HasSubstr;
using ::differential_privacy::base::testing::StatusIs;

const char kSeedString[] = "ABCDEFGHIJKLMNOP";
constexpr int64_t kStatsSize = 50000;
constexpr double kTolerance = 1e-5;

TEST(XorStringsTest, XorsSameLength) {
  std::string first = "foo";
  std::string second = "bar";

  std::string result = XorStrings(first, second);

  EXPECT_EQ('f' ^ 'b', result[0]);
  EXPECT_EQ('o' ^ 'a', result[1]);
  EXPECT_EQ('o' ^ 'r', result[2]);
}

TEST(XorStringsTest, ShorterStringRepeated) {
  std::string first = "foobar";
  std::string second = "baz";

  std::string result = XorStrings(first, second);

  EXPECT_EQ('b' ^ 'b', result[3]);
  EXPECT_EQ('a' ^ 'a', result[4]);
  EXPECT_EQ('z' ^ 'r', result[5]);
}

TEST(XorStringsTest, EmptyStringReturnsUnchanged) {
  std::string first = "foo";
  std::string second = "";

  std::string result = XorStrings(first, second);

  EXPECT_EQ(result, "foo");
}

TEST(XorStringsTest, DoubleEmptyString) {
  std::string first = "";
  std::string second = "";

  std::string result = XorStrings(first, second);

  EXPECT_EQ(result, "");
}

TEST(EpsilonRiskValuesTest, DefaultEpsilon) {
  EXPECT_EQ(DefaultEpsilon(), std::log(3));
}

TEST(NextPowerTest, PositivesPowers) {
  EXPECT_NEAR(GetNextPowerOfTwo(3.0), 4.0, kTolerance);
  EXPECT_NEAR(GetNextPowerOfTwo(5.0), 8.0, kTolerance);
  EXPECT_NEAR(GetNextPowerOfTwo(7.9), 8.0, kTolerance);
}

TEST(NextPowerTest, ExactPositivePowers) {
  EXPECT_NEAR(GetNextPowerOfTwo(2.0), 2.0, kTolerance);
  EXPECT_NEAR(GetNextPowerOfTwo(8.0), 8.0, kTolerance);
}

TEST(NextPowerTest, One) {
  EXPECT_NEAR(GetNextPowerOfTwo(1.0), 1.0, kTolerance);
}

TEST(NextPowerTest, NegativePowers) {
  EXPECT_NEAR(GetNextPowerOfTwo(0.4), 0.5, kTolerance);
  EXPECT_NEAR(GetNextPowerOfTwo(0.2), 0.25, kTolerance);
}

TEST(NextPowerTest, ExactNegativePowers) {
  EXPECT_NEAR(GetNextPowerOfTwo(0.5), 0.5, kTolerance);
  EXPECT_NEAR(GetNextPowerOfTwo(0.125), 0.125, kTolerance);
}

TEST(InverseErrorTest, ProperResults) {
  // true values are pre-calculated
  EXPECT_NEAR(InverseErrorFunction(0.24), 0.216, 0.001);
  EXPECT_NEAR(InverseErrorFunction(0.9999), 2.751, 0.001);
  EXPECT_NEAR(InverseErrorFunction(0.0012), 0.001, 0.001);
  EXPECT_NEAR(InverseErrorFunction(0.5), 0.476, 0.001);
  EXPECT_NEAR(InverseErrorFunction(0.39), 0.360, 0.001);
  EXPECT_NEAR(InverseErrorFunction(0.0067), 0.0059, 0.001);

  double max = 1;
  double min = -1;
  for (int i = 0; i < 1000; i++) {
    double n = (max - min) * ((double)rand() / RAND_MAX) + min;
    EXPECT_NEAR(std::erf(InverseErrorFunction(n)), n, 0.001);
  }
}

TEST(InverseErrorTest, EdgeCases) {
  EXPECT_EQ(InverseErrorFunction(-1),
            -1 * std::numeric_limits<double>::infinity());
  EXPECT_EQ(InverseErrorFunction(1), std::numeric_limits<double>::infinity());
  EXPECT_EQ(InverseErrorFunction(0), 0);
}

// In RoundToNearestMultiple tests exact comparison of double is used, because
// for rounding to multiple of power of 2 RoundToNearestMultiple should provide
// exact value.
TEST(RoundTest, PositiveNoTies) {
  EXPECT_EQ(RoundToNearestMultiple(4.9, 2.0), 4.0);
  EXPECT_EQ(RoundToNearestMultiple(5.1, 2.0), 6.0);
}

TEST(RoundTest, NegativesNoTies) {
  EXPECT_EQ(RoundToNearestMultiple(-4.9, 2.0), -4.0);
  EXPECT_EQ(RoundToNearestMultiple(-5.1, 2.0), -6.0);
}

TEST(RoundTest, PositiveTies) {
  EXPECT_EQ(RoundToNearestMultiple(5.0, 2.0), 6.0);
}

TEST(RoundTest, NegativeTies) {
  EXPECT_EQ(RoundToNearestMultiple(-5.0, 2.0), -4.0);
}

TEST(RoundTest, NegativePowerOf2) {
  EXPECT_EQ(RoundToNearestMultiple(0.2078795763, 0.25), 0.25);
  EXPECT_EQ(RoundToNearestMultiple(0.1, 1.0 / (1 << 10)), 0.099609375);
  EXPECT_EQ(RoundToNearestMultiple(0.3, 1.0 / (1 << 30)),
            322122547.0 / (1 << 30));
}

TEST(QnormTest, InvalidProbability) {
  EXPECT_EQ(Qnorm(-0.1).status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(Qnorm(0).status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(Qnorm(1).status().code(), absl::StatusCode::kInvalidArgument);
  EXPECT_EQ(Qnorm(2).status().code(), absl::StatusCode::kInvalidArgument);
}
TEST(QnormTest, Accuracy) {
  double theoretical_accuracy = 4.5 * std::pow(10, -4);
  std::vector<double> p = {0.0000001, 0.00001, 0.001,   0.05,     0.15, 0.25,
                           0.35,      0.45,    0.55,    0.65,     0.75, 0.85,
                           0.95,      0.999,   0.99999, 0.9999999};
  std::vector<double> exact = {
      -5.199337582187471,   -4.264890793922602,   -3.090232306167813,
      -1.6448536269514729,  -1.0364333894937896,  -0.6744897501960817,
      -0.38532046640756773, -0.12566134685507402, 0.12566134685507402,
      0.38532046640756773,  0.6744897501960817,   1.0364333894937896,
      1.6448536269514729,   3.090232306167813,    4.264890793922602,
      5.199337582187471};
  for (int i = 0; i < p.size(); ++i) {
    EXPECT_LE(std::abs(exact[i] - Qnorm(p[i]).ValueOrDie()),
              theoretical_accuracy);
  }
}

TEST(ClampTest, DefaultTest) {
  EXPECT_EQ(Clamp(1, 3, 2), 2);
  EXPECT_EQ(Clamp(1.0, 3.0, 4.0), 3);
  EXPECT_EQ(Clamp(1.0, 3.0, -2.0), 1);
}

TEST(SafeOperationsTest, SafeAddInt) {
  int64_t int_result;
  EXPECT_TRUE(SafeAdd<int64_t>(10, 20, &int_result));
  EXPECT_EQ(int_result, 30);
  EXPECT_TRUE(SafeAdd<int64_t>(std::numeric_limits<int64_t>::max(),
                             std::numeric_limits<int64_t>::lowest(),
                             &int_result));
  EXPECT_EQ(int_result, -1);
  EXPECT_FALSE(
      SafeAdd<int64_t>(std::numeric_limits<int64_t>::max(), 1, &int_result));
  EXPECT_EQ(int_result, std::numeric_limits<int64_t>::max());
  EXPECT_FALSE(
      SafeAdd<int64_t>(std::numeric_limits<int64_t>::lowest(), -1, &int_result));
  EXPECT_EQ(int_result, std::numeric_limits<int64_t>::lowest());
  EXPECT_TRUE(
      SafeAdd<int64_t>(std::numeric_limits<int64_t>::lowest(), 0, &int_result));
  EXPECT_EQ(int_result, std::numeric_limits<int64_t>::lowest());
}

TEST(SafeOperationsTest, SafeAddDouble) {
  double double_result;
  EXPECT_TRUE(SafeAdd<double>(10, 20, &double_result));
  EXPECT_EQ(double_result, 30);
  EXPECT_TRUE(SafeAdd<double>(std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::lowest(),
                              &double_result));
  EXPECT_FLOAT_EQ(double_result, 0);
  EXPECT_TRUE(
      SafeAdd<double>(std::numeric_limits<double>::max(), 1.0, &double_result));
  EXPECT_FLOAT_EQ(double_result, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(SafeAdd<double>(std::numeric_limits<double>::lowest(), -1.0,
                              &double_result));
  EXPECT_FLOAT_EQ(double_result, -std::numeric_limits<double>::infinity());
  EXPECT_TRUE(SafeAdd<double>(std::numeric_limits<double>::lowest(), 0.0,
                              &double_result));
  EXPECT_EQ(double_result, std::numeric_limits<double>::lowest());
}

TEST(SafeOperationsTest, SafeSubtractInt) {
  int64_t int_result;
  EXPECT_TRUE(SafeSubtract<int64_t>(10, 20, &int_result));
  EXPECT_EQ(int_result, -10);
  EXPECT_FALSE(SafeSubtract<int64_t>(1, std::numeric_limits<int64_t>::lowest(),
                                   &int_result));
  EXPECT_EQ(int_result, std::numeric_limits<int64_t>::lowest());
  EXPECT_TRUE(SafeSubtract<int64_t>(-1, std::numeric_limits<int64_t>::lowest(),
                                  &int_result));
  EXPECT_EQ(int_result, std::numeric_limits<int64_t>::max());
  EXPECT_TRUE(SafeSubtract<int64_t>(std::numeric_limits<int64_t>::lowest(),
                                  std::numeric_limits<int64_t>::lowest(),
                                  &int_result));
  EXPECT_EQ(int_result, 0);

  uint64_t uint_result;
  EXPECT_TRUE(SafeSubtract<uint64_t>(1, std::numeric_limits<uint64_t>::lowest(),
                                   &uint_result));
  EXPECT_EQ(uint_result, 1);
}

TEST(SafeOperationsTest, SafeSubtractDouble) {
  double double_result;
  EXPECT_TRUE(SafeSubtract<double>(10.0, 20.0, &double_result));
  EXPECT_DOUBLE_EQ(double_result, -10.0);
  EXPECT_TRUE(SafeSubtract<double>(1.0, std::numeric_limits<double>::lowest(),
                                   &double_result));
  EXPECT_DOUBLE_EQ(double_result, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(SafeSubtract<double>(-1.0, std::numeric_limits<double>::lowest(),
                                   &double_result));
  EXPECT_DOUBLE_EQ(double_result, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(SafeSubtract<double>(std::numeric_limits<double>::lowest(),
                                   std::numeric_limits<double>::lowest(),
                                   &double_result));
  EXPECT_EQ(double_result, 0);
}

TEST(SafeOperationsTest, SafeSquare) {
  int64_t int_result;
  EXPECT_TRUE(SafeSquare<int64_t>(-9, &int_result));
  EXPECT_EQ(int_result, 81);
  EXPECT_FALSE(
      SafeSquare<int64_t>(std::numeric_limits<int64_t>::max() - 1, &int_result));
  EXPECT_FALSE(
      SafeSquare<int64_t>(std::numeric_limits<int64_t>::lowest() + 1, &int_result));
  EXPECT_FALSE(
      SafeSquare<int64_t>(std::numeric_limits<int64_t>::lowest(), &int_result));

  uint64_t uint_result;
  EXPECT_TRUE(
      SafeSquare<uint64_t>(std::numeric_limits<uint64_t>::lowest(), &uint_result));
}

TEST(StatisticsTest, VectorStatistics) {
  std::vector<double> a = {1, 5, 7, 9, 13};
  EXPECT_EQ(Mean(a), 7);
  EXPECT_EQ(Variance(a), 16);
  EXPECT_EQ(StandardDev(a), 4);
  EXPECT_EQ(OrderStatistic(.60, a), 8);
  EXPECT_EQ(OrderStatistic(0, a), 1);
  EXPECT_EQ(OrderStatistic(1, a), 13);
}

TEST(VectorUtilTest, VectorFilter) {
  std::vector<double> v = {1, 2, 2, 3};
  std::vector<bool> selection = {false, true, true, false};
  std::vector<double> expected = {2, 2};
  EXPECT_THAT(VectorFilter(v, selection), testing::ContainerEq(expected));
}

TEST(VectorUtilTest, VectorToString) {
  std::vector<double> v = {1, 2, 2, 3};
  EXPECT_EQ(VectorToString(v), "[1, 2, 2, 3]");
}

TEST(SafeCastFromDoubleTest, Converts20ToIntegral) {
  int64_t integral = 345;
  EXPECT_TRUE(SafeCastFromDouble(20.0, integral));
  EXPECT_EQ(integral, 20);
}

TEST(SafeCastFromDoubleTest, ConvertsHighValueToMaxIntegral) {
  int64_t integral = 345;
  EXPECT_TRUE(SafeCastFromDouble(1.0e200, integral));
  EXPECT_EQ(integral, std::numeric_limits<int64_t>::max());
}

TEST(SafeCastFromDoubleTest, ConvertsLowValueToLowestIntegral) {
  int64_t integral = 345;
  EXPECT_TRUE(SafeCastFromDouble(-1.0e200, integral));
  EXPECT_EQ(integral, std::numeric_limits<int64_t>::lowest());
}

TEST(SafeCastFromDoubleTest, ReturnsFalseOnNanForIntegrals) {
  int64_t integral = 345;
  EXPECT_FALSE(SafeCastFromDouble(NAN, integral));
  EXPECT_EQ(integral, 345);
}

// Combine all tests for float outputs.  Should be nothing unexpected here since
// this is just a cast from double to float.
TEST(SafeCastFromDoubleTest, ForFloat) {
  float floating_point;

  // Normal case.
  EXPECT_TRUE(SafeCastFromDouble(0.5, floating_point));
  EXPECT_EQ(floating_point, 0.5);

  // NaN double should convert into NaN float.
  EXPECT_TRUE(SafeCastFromDouble(NAN, floating_point));
  EXPECT_TRUE(std::isnan(floating_point));

  // High double should convert into infinite float.
  EXPECT_TRUE(SafeCastFromDouble(1.0e200, floating_point));
  EXPECT_TRUE(std::isinf(floating_point));
}

TEST(ValidateTest, IsSet) {
  absl::optional<double> opt;
  EXPECT_THAT(ValidateIsSet(opt, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be set.")));

  opt = std::numeric_limits<double>::quiet_NaN();
  EXPECT_THAT(ValidateIsSet(opt, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be a valid numeric value")));

  std::vector<double> success_values = {
      -std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::lowest(),
      -1,
      0,
      std::numeric_limits<double>::min(),
      1,
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::infinity()};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsSet(value, "Test value"));
  }
}

TEST(ValidateTest, IsPositive) {
  std::vector<double> success_values = {
      std::numeric_limits<double>::min(), 1, std::numeric_limits<double>::max(),
      std::numeric_limits<double>::infinity()};
  std::vector<double> error_values = {-std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::lowest(),
                                      -10, -1, 0};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsPositive(value, "Test value"));
  }

  for (double value : error_values) {
    EXPECT_THAT(ValidateIsPositive(value, "Test value"),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("Test value must be positive")));
  }
}

TEST(ValidateTest, IsNonNegative) {
  std::vector<double> success_values = {
      0, std::numeric_limits<double>::min(), 1,
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::infinity()};
  std::vector<double> error_values = {-std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::lowest(),
                                      -10, -1};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsNonNegative(value, "Test value"));
  }

  for (double value : error_values) {
    EXPECT_THAT(ValidateIsNonNegative(value, "Test value"),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("Test value must be non-negative")));
  }
}

TEST(ValidateTest, IsFinite) {
  std::vector<double> success_values = {std::numeric_limits<double>::lowest(),
                                        -1,
                                        0,
                                        std::numeric_limits<double>::min(),
                                        1,
                                        std::numeric_limits<double>::max()};

  std::vector<double> error_values = {-std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::infinity()};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsFinite(value, "Test value"));
  }

  for (double value : error_values) {
    EXPECT_THAT(ValidateIsFinite(value, "Test value"),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("Test value must be finite")));
  }
}

TEST(ValidateTest, IsFiniteAndPositive) {
  std::vector<double> success_values = {std::numeric_limits<double>::min(), 1,
                                        std::numeric_limits<double>::max()};
  std::vector<double> error_values = {-std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::lowest(),
                                      -10,
                                      -1,
                                      0,
                                      std::numeric_limits<double>::infinity()};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsFiniteAndPositive(value, "Test value"));
  }

  for (double value : error_values) {
    EXPECT_THAT(ValidateIsFiniteAndPositive(value, "Test value"),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("Test value must be finite and positive")));
  }
}

TEST(ValidateTest, IsFiniteAndNonNegative) {
  std::vector<double> success_values = {0, std::numeric_limits<double>::min(),
                                        1, std::numeric_limits<double>::max()};
  std::vector<double> error_values = {-std::numeric_limits<double>::infinity(),
                                      std::numeric_limits<double>::lowest(),
                                      -10, -1,
                                      std::numeric_limits<double>::infinity()};

  for (double value : success_values) {
    EXPECT_OK(ValidateIsFiniteAndNonNegative(value, "Test value"));
  }

  for (double value : error_values) {
    EXPECT_THAT(
        ValidateIsFiniteAndNonNegative(value, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be finite and non-negative")));
  }
}

TEST(ValidateTest, IsLesserThanOkStatus) {
  struct LesserThanParams {
    double value;
    double upper_bound;
  };

  std::vector<LesserThanParams> success_params = {
      {-std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::lowest()},
      {-1, 1},
      {0, std::numeric_limits<double>::min()},
      {std::numeric_limits<double>::max(),
       std::numeric_limits<double>::infinity()},
  };

  for (LesserThanParams params : success_params) {
    EXPECT_OK(
        ValidateIsLesserThan(params.value, params.upper_bound, "Test value"));
  }
}

TEST(ValidateTest, IsLesserThanError) {
  struct LesserThanParams {
    double value;
    double upper_bound;
  };

  std::vector<LesserThanParams> no_equal_error_params = {
      {-std::numeric_limits<double>::infinity(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest()},
      {-1, -1},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min()},
      {0, 0},
      {1, -1},
      {1, 1},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::infinity()}};

  for (LesserThanParams params : no_equal_error_params) {
    EXPECT_THAT(
        ValidateIsLesserThan(params.value, params.upper_bound, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be lesser than")));
  }
}

TEST(ValidateTest, IsLesserThanOrEqualToOkStatus) {
  struct LesserThanParams {
    double value;
    double upper_bound;
  };

  std::vector<LesserThanParams> success_params = {
      {-std::numeric_limits<double>::infinity(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest()},
      {-1, -1},
      {-1, 1},
      {0, 0},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min()},
      {
          1,
          1,
      },
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::infinity()}};

  for (LesserThanParams params : success_params) {
    EXPECT_OK(ValidateIsLesserThanOrEqualTo(params.value, params.upper_bound,
                                            "Test value"));
  }
}

TEST(ValidateTest, IsLesserThanOrEqualToError) {
  struct LesserThanParams {
    double value;
    double upper_bound;
  };

  std::vector<LesserThanParams> or_equal_error_params = {
      {std::numeric_limits<double>::lowest(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::min(), 0},
      {1, -1},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::max()}};

  for (LesserThanParams params : or_equal_error_params) {
    EXPECT_THAT(
        ValidateIsLesserThanOrEqualTo(params.value, params.upper_bound,
                                      "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be lesser than or equal to")));
  }
}

TEST(ValidateTest, IsGreaterThanOkStatus) {
  struct GreaterThanParams {
    double value;
    double lower_bound;
  };

  std::vector<GreaterThanParams> success_params = {
      {std::numeric_limits<double>::lowest(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::min(), 0},
      {1, -1},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::max()},
  };

  for (GreaterThanParams params : success_params) {
    EXPECT_OK(
        ValidateIsGreaterThan(params.value, params.lower_bound, "Test value"));
  }
}

TEST(ValidateTest, IsGreaterThanError) {
  struct GreaterThanParams {
    double value;
    double lower_bound;
  };

  std::vector<GreaterThanParams> no_equal_error_params = {
      {-std::numeric_limits<double>::infinity(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest()},
      {-1, -1},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min()},
      {0, 0},
      {-1, 1},
      {1, 1},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::infinity()}};

  for (GreaterThanParams params : no_equal_error_params) {
    EXPECT_THAT(
        ValidateIsGreaterThan(params.value, params.lower_bound, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be greater than")));
  }
}

TEST(ValidateTest, IsGreaterThanOrEqualToOkStatus) {
  struct GreaterThanParams {
    double value;
    double lower_bound;
  };

  std::vector<GreaterThanParams> success_params = {
      {-std::numeric_limits<double>::infinity(),
       -std::numeric_limits<double>::infinity()},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest()},
      {-1, -1},
      {0, 0},
      {1, -1},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min()},
      {1, 1},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max()},
      {std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::infinity()}};

  for (GreaterThanParams params : success_params) {
    EXPECT_OK(ValidateIsGreaterThanOrEqualTo(params.value, params.lower_bound,
                                             "Test value"));
  }
}

TEST(ValidateTest, IsGreaterThanOrEqualToError) {
  struct GreaterThanParams {
    double value;
    double lower_bound;
  };

  std::vector<GreaterThanParams> or_equal_error_params = {
      {-std::numeric_limits<double>::infinity(),
       std::numeric_limits<double>::lowest()},
      {0, std::numeric_limits<double>::min()},
      {-1, 1},
      {std::numeric_limits<double>::max(),
       std::numeric_limits<double>::infinity()}};

  for (GreaterThanParams params : or_equal_error_params) {
    EXPECT_THAT(
        ValidateIsGreaterThanOrEqualTo(params.value, params.lower_bound,
                                       "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be greater than or equal to")));
  }
}

TEST(ValidateTest, IsInIntervalOkStatus) {
  struct IntervalParams {
    double value;
    double lower_bound;
    double upper_bound;
    bool include_lower;
    bool include_upper;
  };

  std::vector<IntervalParams> success_params = {
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(), false, true},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(), true, false},
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(), true, true},
      {0, -1, 1, false, false},
      {0, -1, 1, true, false},
      {0, -1, 1, false, true},
      {0, -1, 1, true, true},
      {0, 0, 0, false, true},
      {0, 0, 0, true, false},
      {0, 0, 0, true, true},
      {0.0, 0.0 - std::numeric_limits<double>::min(),
       0.0 + std::numeric_limits<double>::min(), false, false},
      {-1, -1, 1, true, false},
      {1, -1, 1, false, true},
      {1, 1, 1, false, true},
      {1, 1, 1, true, false},
      {1, 1, 1, true, true},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min(),
       std::numeric_limits<double>::min(), false, true},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min(),
       std::numeric_limits<double>::min(), true, false},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min(),
       std::numeric_limits<double>::min(), true, true},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
       std::numeric_limits<double>::max(), false, true},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
       std::numeric_limits<double>::max(), true, false},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
       std::numeric_limits<double>::max(), true, true},
  };

  for (IntervalParams params : success_params) {
    EXPECT_OK(ValidateIsInInterval(params.value, params.lower_bound,
                                   params.upper_bound, params.include_lower,
                                   params.include_upper, "Test value"));
  }
}

TEST(ValidateTest, IsOutsideExclusiveInterval) {
  struct IntervalParams {
    double value;
    double lower_bound;
    double upper_bound;
    bool include_lower;
    bool include_upper;
  };

  std::vector<IntervalParams> exclusive_error_params = {
      {std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(),
       std::numeric_limits<double>::lowest(), false, false},
      {-1, 0, 1, false, false},
      {-1, -1, -1, false, false},
      {0, 0, 0, false, false},
      {1, 1, 1, false, false},
      {std::numeric_limits<double>::min(), std::numeric_limits<double>::min(),
       std::numeric_limits<double>::min(), false, false},
      {std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
       std::numeric_limits<double>::max(), false, false},
  };

  for (IntervalParams params : exclusive_error_params) {
    EXPECT_THAT(
        ValidateIsInInterval(params.value, params.lower_bound,
                             params.upper_bound, params.include_lower,
                             params.include_upper, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be in the exclusive interval (")));
  }
}

TEST(ValidateTest, IsOutsideInclusiveInterval) {
  struct IntervalParams {
    double value;
    double lower_bound;
    double upper_bound;
    bool include_lower;
    bool include_upper;
  };

  std::vector<IntervalParams> inclusive_error_params = {
      {-1, 0, 1, true, true},
      {0 - std::numeric_limits<double>::min(), 0,
       std::numeric_limits<double>::min(), true, true},
  };

  for (IntervalParams params : inclusive_error_params) {
    EXPECT_THAT(
        ValidateIsInInterval(params.value, params.lower_bound,
                             params.upper_bound, params.include_lower,
                             params.include_upper, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be in the inclusive interval [")));
  }
}

TEST(ValidateTest, IsOutsideHalfClosedInterval) {
  struct IntervalParams {
    double value;
    double lower_bound;
    double upper_bound;
    bool include_lower;
    bool include_upper;
  };

  EXPECT_THAT(ValidateIsInInterval(-1, 0, 1, true, false, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be in the interval [0,1)")));

  EXPECT_THAT(ValidateIsInInterval(-1, 0, 1, false, true, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be in the interval (0,1]")));

  EXPECT_THAT(ValidateIsInInterval(-1, -1, 1, false, true, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be in the interval (-1,1]")));

  EXPECT_THAT(ValidateIsInInterval(1, -1, 1, true, false, "Test value"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Test value must be in the interval [-1,1)")));
}

// These tests document cases that result in known, incorrect behaviour
TEST(ValidateTest, IsInIntervalBadBehaviour) {
  struct IntervalParams {
    double value;
    double lower_bound;
    double upper_bound;
    bool include_lower;
    bool include_upper;
  };

  std::vector<IntervalParams> bad_exclusive_error_params = {
      // These test parameters should result in an OK_STATUS since the value is
      // within the bounds, but instead returns a kInvalidArgument status
      // because of double (im)precision.
      {-1.0, -1.0 - std::numeric_limits<double>::min(),
       -1.0 + std::numeric_limits<double>::min(), false, false},
      {1.0, 1.0 - std::numeric_limits<double>::min(),
       1.0 + std::numeric_limits<double>::min(), false, false},
  };

  for (IntervalParams params : bad_exclusive_error_params) {
    EXPECT_THAT(
        ValidateIsInInterval(params.value, params.lower_bound,
                             params.upper_bound, params.include_lower,
                             params.include_upper, "Test value"),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Test value must be in the exclusive interval (")));
  }

  std::vector<IntervalParams> bad_success_params = {
      // These test parameters should result in an kInvalidArgument status since
      // the value falls outside of the bounds, but instead returns an OK_STATUS
      // because of double (im)precision.
      {-1.0 - std::numeric_limits<double>::min(), -1.0,
       -1.0 + std::numeric_limits<double>::min(), true, true},
      {1.0 - std::numeric_limits<double>::min(), 1.0,
       1.0 + std::numeric_limits<double>::min(), true, true},
  };

  for (IntervalParams params : bad_success_params) {
    EXPECT_OK(ValidateIsInInterval(params.value, params.lower_bound,
                                   params.upper_bound, params.include_lower,
                                   params.include_upper, "Test value"));
  }
}

}  // namespace
}  // namespace differential_privacy
