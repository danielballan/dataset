/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "dimensions.h"
#include "tags.h"
#include "variable.h"

TEST(Variable, construct) {
  ASSERT_NO_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2));
  const auto a = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto &data = a.get<const Data::Value>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(), 2));
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 1), 2));
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 3), 2));
}

TEST(Variable, span_references_Variable) {
  auto a = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  auto observer = a.get<const Data::Value>();
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  // Note: This also has the "usual" problem of copy-on-write: This non-const
  // call can invalidate the references stored in observer if the underlying
  // data was shared.
  auto span = a.get<Data::Value>();

  EXPECT_EQ(span.size(), 2);
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

TEST(Variable, sharing) {
  const auto a1 = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto a2(a1);
  // TODO Should we require the use of `const` with the tag if Variable is
  // const?
  EXPECT_EQ(&a1.get<const Data::Value>()[0], &a2.get<const Data::Value>()[0]);
}

TEST(Variable, copy) {
  const auto a1 =
      makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  const auto &data1 = a1.get<const Data::Value>();
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_EQ(&a1.get<const Data::Value>()[0], &a2.get<const Data::Value>()[0]);
  EXPECT_NE(&a1.get<const Data::Value>()[0], &a2.get<Data::Value>()[0]);
  const auto &data2 = a2.get<Data::Value>();
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}

TEST(Variable, operator_equals) {
  const auto a = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.2});
  const auto a_copy(a);
  const auto b = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.2});
  const auto diff1 = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.1});
  const auto diff2 = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});
  auto diff3(a);
  diff3.setName("test");
  auto diff4(a);
  diff4.setUnit(Unit::Id::Length);
  EXPECT_EQ(a, a);
  EXPECT_EQ(a, a_copy);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);
  EXPECT_FALSE(a == diff1);
  EXPECT_FALSE(a == diff2);
  EXPECT_FALSE(a == diff3);
  EXPECT_FALSE(a == diff4);
}

TEST(Variable, operator_plus_equal) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.2);
  EXPECT_EQ(a.get<Data::Value>()[1], 4.4);

  auto different_name(a);
  different_name.setName("test");
  EXPECT_NO_THROW(a += different_name);
}

TEST(Variable, operator_plus_equal_automatic_broadcast_of_rhs) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto fewer_dimensions = makeVariable<Data::Value>({}, {1.0});

  EXPECT_NO_THROW(a += fewer_dimensions);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.1);
  EXPECT_EQ(a.get<Data::Value>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_transpose) {
  auto a = makeVariable<Data::Value>(
      Dimensions({{Dimension::Y, 3}, {Dimension::X, 2}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  auto transpose = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 2}, {Dimension::Y, 3}}),
      {1.0, 3.0, 5.0, 2.0, 4.0, 6.0});

  EXPECT_NO_THROW(a += transpose);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.0);
  EXPECT_EQ(a.get<Data::Value>()[1], 4.0);
  EXPECT_EQ(a.get<Data::Value>()[2], 6.0);
  EXPECT_EQ(a.get<Data::Value>()[3], 8.0);
  EXPECT_EQ(a.get<Data::Value>()[4], 10.0);
  EXPECT_EQ(a.get<Data::Value>()[5], 12.0);
}

TEST(Variable, operator_plus_equal_different_dimensions) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto different_dimensions =
      makeVariable<Data::Value>({Dimension::Y, 2}, {1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Cannot add Variables: Dimensions do not match.");
}

TEST(Variable, operator_plus_equal_different_unit) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto different_unit(a);
  different_unit.setUnit(Unit::Id::Length);
  EXPECT_THROW_MSG(a += different_unit, std::runtime_error,
                   "Cannot add Variables: Units do not match.");
}

TEST(Variable, operator_plus_equal_non_arithmetic_type) {
  auto a = makeVariable<Data::String>({Dimension::X, 1}, {std::string("test")});
  EXPECT_THROW_MSG(a += a, std::runtime_error,
                   "Cannot add strings. Use append() instead.");
}

TEST(Variable, operator_plus_equal_different_variables_different_element_type) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto b = makeVariable<Data::Int>({Dimension::X, 1}, {2});
  EXPECT_THROW_MSG(a += b, std::runtime_error,
                   "Cannot apply arithmetic operation to Variables: Underlying "
                   "data types do not match.");
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto b = makeVariable<Data::Variance>({Dimension::X, 1}, {2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get<Data::Value>()[0], 3.0);
}

TEST(Variable, operator_times_equal) {
  auto a = makeVariable<Coord::X>({Dimension::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), Unit::Id::Length);
  EXPECT_NO_THROW(a *= a);
  EXPECT_EQ(a.get<Data::Value>()[0], 4.0);
  EXPECT_EQ(a.get<Data::Value>()[1], 9.0);
  EXPECT_EQ(a.unit(), Unit::Id::Area);
}

TEST(Variable, setSlice) {
  Dimensions dims(Dimension::Tof, 1);
  const auto parent = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});
  const auto empty = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      24);

  auto d(empty);
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1, 2, 3})
    d.setSlice(slice(parent, Dimension::X, index), Dimension::X, index);
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1})
    d.setSlice(slice(parent, Dimension::Y, index), Dimension::Y, index);
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1, 2})
    d.setSlice(slice(parent, Dimension::Z, index), Dimension::Z, index);
  EXPECT_EQ(parent, d);
}

TEST(Variable, slice) {
  Dimensions dims(Dimension::Tof, 1);
  const auto parent = makeVariable<Data::Value>(
      Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}, {Dimension::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const gsl::index index : {0, 1, 2, 3}) {
    auto sliceX = slice(parent, Dimension::X, index);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}}));
    EXPECT_EQ(sliceX.get<const Data::Value>()[0], index + 1.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[1], index + 5.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[2], index + 9.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[3], index + 13.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[4], index + 17.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[5], index + 21.0);
  }

  for (const gsl::index index : {0, 1}) {
    auto sliceY = slice(parent, Dimension::Y, index);
    ASSERT_EQ(sliceY.dimensions(),
              Dimensions({{Dimension::Z, 3}, {Dimension::X, 4}}));
    const auto &data = sliceY.get<const Data::Value>();
    for (const gsl::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * index + 8 * z + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * index + 8 * z + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * index + 8 * z + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * index + 8 * z + 4.0);
    }
  }

  for (const gsl::index index : {0, 1, 2}) {
    auto sliceZ = slice(parent, Dimension::Z, index);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dimension::Y, 2}, {Dimension::X, 4}}));
    const auto &data = sliceZ.get<const Data::Value>();
    for (gsl::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }
}

TEST(Variable, slice_range) {
  Dimensions dims(Dimension::Tof, 1);
  const auto parent = makeVariable<Data::Value>(
      Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}, {Dimension::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const gsl::index index : {0, 1, 2, 3}) {
    auto sliceX = slice(parent, Dimension::X, index, index + 1);
    ASSERT_EQ(
        sliceX.dimensions(),
        Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}, {Dimension::X, 1}}));
    EXPECT_EQ(sliceX.get<const Data::Value>()[0], index + 1.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[1], index + 5.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[2], index + 9.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[3], index + 13.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[4], index + 17.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[5], index + 21.0);
  }

  for (const gsl::index index : {0, 1, 2}) {
    auto sliceX = slice(parent, Dimension::X, index, index + 2);
    ASSERT_EQ(
        sliceX.dimensions(),
        Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}, {Dimension::X, 2}}));
    EXPECT_EQ(sliceX.get<const Data::Value>()[0], index + 1.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[1], index + 2.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[2], index + 5.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[3], index + 6.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[4], index + 9.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[5], index + 10.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[6], index + 13.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[7], index + 14.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[8], index + 17.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[9], index + 18.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[10], index + 21.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[11], index + 22.0);
  }

  for (const gsl::index index : {0, 1}) {
    auto sliceY = slice(parent, Dimension::Y, index, index + 1);
    ASSERT_EQ(
        sliceY.dimensions(),
        Dimensions({{Dimension::Z, 3}, {Dimension::Y, 1}, {Dimension::X, 4}}));
    const auto &data = sliceY.get<const Data::Value>();
    for (const gsl::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * index + 8 * z + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * index + 8 * z + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * index + 8 * z + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * index + 8 * z + 4.0);
    }
  }

  for (const gsl::index index : {0}) {
    auto sliceY = slice(parent, Dimension::Y, index, index + 2);
    EXPECT_EQ(sliceY, parent);
  }

  for (const gsl::index index : {0, 1, 2}) {
    auto sliceZ = slice(parent, Dimension::Z, index, index + 1);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dim::Z, 1}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.get<const Data::Value>();
    for (gsl::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }

  for (const gsl::index index : {0, 1}) {
    auto sliceZ = slice(parent, Dimension::Z, index, index + 2);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dim::Z, 2}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.get<const Data::Value>();
    for (gsl::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
    for (gsl::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[8 + xy], 1.0 + 8 + xy + 8 * index);
  }
}

TEST(Variable, concatenate) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b = makeVariable<Data::Value>(dims, {2.0});
  a.setUnit(Unit::Id::Length);
  b.setUnit(Unit::Id::Length);
  auto ab = concatenate(a, b, Dimension::Tof);
  ASSERT_EQ(ab.size(), 2);
  EXPECT_EQ(ab.unit(), Unit(Unit::Id::Length));
  const auto &data = ab.get<Data::Value>();
  EXPECT_EQ(data[0], 1.0);
  EXPECT_EQ(data[1], 2.0);
  auto ba = concatenate(b, a, Dimension::Tof);
  const auto abba = concatenate(ab, ba, Dimension::Q);
  ASSERT_EQ(abba.size(), 4);
  EXPECT_EQ(abba.dimensions().count(), 2);
  const auto &data2 = abba.get<const Data::Value>();
  EXPECT_EQ(data2[0], 1.0);
  EXPECT_EQ(data2[1], 2.0);
  EXPECT_EQ(data2[2], 2.0);
  EXPECT_EQ(data2[3], 1.0);
  const auto ababbaba = concatenate(abba, abba, Dimension::Tof);
  ASSERT_EQ(ababbaba.size(), 8);
  const auto &data3 = ababbaba.get<const Data::Value>();
  EXPECT_EQ(data3[0], 1.0);
  EXPECT_EQ(data3[1], 2.0);
  EXPECT_EQ(data3[2], 1.0);
  EXPECT_EQ(data3[3], 2.0);
  EXPECT_EQ(data3[4], 2.0);
  EXPECT_EQ(data3[5], 1.0);
  EXPECT_EQ(data3[6], 2.0);
  EXPECT_EQ(data3[7], 1.0);
  const auto abbaabba = concatenate(abba, abba, Dimension::Q);
  ASSERT_EQ(abbaabba.size(), 8);
  const auto &data4 = abbaabba.get<const Data::Value>();
  EXPECT_EQ(data4[0], 1.0);
  EXPECT_EQ(data4[1], 2.0);
  EXPECT_EQ(data4[2], 2.0);
  EXPECT_EQ(data4[3], 1.0);
  EXPECT_EQ(data4[4], 1.0);
  EXPECT_EQ(data4[5], 2.0);
  EXPECT_EQ(data4[6], 2.0);
  EXPECT_EQ(data4[7], 1.0);
}

TEST(Variable, concatenate_volume_with_slice) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dimension::X);
  EXPECT_NO_THROW(concatenate(aa, a, Dimension::X));
}

TEST(Variable, concatenate_slice_with_volume) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dimension::X);
  EXPECT_NO_THROW(concatenate(a, aa, Dimension::X));
}

TEST(Variable, concatenate_fail) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b = makeVariable<Data::Value>(dims, {2.0});
  auto c = makeVariable<Data::Variance>(dims, {2.0});
  a.setName("data");
  EXPECT_THROW_MSG(concatenate(a, b, Dimension::Tof), std::runtime_error,
                   "Cannot concatenate Variables: Names do not match.");
  c.setName("data");
  EXPECT_THROW_MSG(concatenate(a, c, Dimension::Tof), std::runtime_error,
                   "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(a, a, Dimension::Tof);
  EXPECT_THROW_MSG(
      concatenate(a, aa, Dimension::Q), std::runtime_error,
      "Cannot concatenate Variables: Dimension extents do not match.");
}

TEST(Variable, concatenate_unit_fail) {
  Dimensions dims(Dimension::X, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b(a);
  EXPECT_NO_THROW(concatenate(a, b, Dimension::X));
  a.setUnit(Unit::Id::Length);
  EXPECT_THROW_MSG(concatenate(a, b, Dimension::X), std::runtime_error,
                   "Cannot concatenate Variables: Units do not match.");
  b.setUnit(Unit::Id::Length);
  EXPECT_NO_THROW(concatenate(a, b, Dimension::X));
}

TEST(Variable, rebin) {
  auto var = makeVariable<Data::Value>({Dim::X, 2}, {1.0, 2.0});
  const auto oldEdge = makeVariable<Coord::X>({Dim::X, 3}, {1.0, 2.0, 3.0});
  const auto newEdge = makeVariable<Coord::X>({Dim::X, 2}, {1.0, 3.0});
  auto rebinned = rebin(var, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dimensions().count(), 1);
  ASSERT_EQ(rebinned.dimensions().volume(), 1);
  ASSERT_EQ(rebinned.get<const Data::Value>().size(), 1);
  EXPECT_EQ(rebinned.get<const Data::Value>()[0], 3.0);
}

TEST(VariableSlice, strides) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
  EXPECT_EQ(var(Dim::X, 0).strides(), (std::vector<gsl::index>{3}));
  EXPECT_EQ(var(Dim::X, 1).strides(), (std::vector<gsl::index>{3}));
  EXPECT_EQ(var(Dim::Y, 0).strides(), (std::vector<gsl::index>{1}));
  EXPECT_EQ(var(Dim::Y, 1).strides(), (std::vector<gsl::index>{1}));
  EXPECT_EQ(var(Dim::X, 0, 1).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 2).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 1).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 2).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 0, 2).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 3).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 2).strides(), (std::vector<gsl::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 3).strides(), (std::vector<gsl::index>{3, 1}));

  EXPECT_EQ(var(Dim::X, 0, 1)(Dim::Y, 0, 1).strides(),
            (std::vector<gsl::index>{3, 1}));

  auto var3D =
      makeVariable<Data::Value>({{Dim::Z, 4}, {Dim::Y, 3}, {Dim::X, 2}});
  EXPECT_EQ(var3D(Dim::X, 0, 1)(Dim::Z, 0, 1).strides(),
            (std::vector<gsl::index>{6, 2, 1}));
}

TEST(VariableSlice, get) {
  const auto var = makeVariable<Data::Value>({Dim::X, 3}, {1, 2, 3});
  EXPECT_EQ(var(Dim::X, 1, 2).get<const Data::Value>()[0], 2.0);
}

TEST(VariableSlice, slicing_does_not_transpose) {
  auto var = makeVariable<Data::Value>({{Dim::X, 3}, {Dim::Y, 3}});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var(Dim::X, 1, 2)(Dim::Y, 1, 2).dimensions(), expected);
  EXPECT_EQ(var(Dim::Y, 1, 2)(Dim::X, 1, 2).dimensions(), expected);
}

TEST(VariableSlice, minus_equals_failures) {
  auto var = makeVariable<Data::Value>({{Dim::X, 2}, {Dim::Y, 2}},
                                       {1.0, 2.0, 3.0, 4.0});

  EXPECT_THROW_MSG(var -= var(Dim::X, 0, 1), std::runtime_error,
                   "Cannot subtract Variables: Dimensions do not match.");
}

TEST(VariableSlice, self_overlapping_view_operation_broken) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});

  var -= var(Dim::Y, 0);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  // This is the broken part: After subtracting for y=0 the view points to data
  // containing 0.0, so subsequently the subtraction will have no effect. Need
  // to check for such overlaps and either throw or extract slice before the
  // operation.
  EXPECT_EQ(data[2], 3.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, minus_equals_slice_const_outer) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});
  const auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  var -= copy(Dim::Y, 1);
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, minus_equals_slice_outer) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  var -= copy(Dim::Y, 1);
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, minus_equals_slice_inner) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 0);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 1.0);
  EXPECT_EQ(data[2], 0.0);
  EXPECT_EQ(data[3], 1.0);
  var -= copy(Dim::X, 1);
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -1.0);
  EXPECT_EQ(data[2], -4.0);
  EXPECT_EQ(data[3], -3.0);
}

TEST(VariableSlice, minus_equals_slice_of_slice) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 1)(Dim::Y, 1);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 0.0);
}

TEST(VariableSlice, minus_equals_nontrivial_slices) {
  auto source = makeVariable<Data::Value>(
      {{Dim::Y, 3}, {Dim::X, 3}},
      {11.0, 12.0, 13.0, 21.0, 22.0, 23.0, 31.0, 32.0, 33.0});
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 0, 2);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], -21.0);
    EXPECT_EQ(data[3], -22.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 0, 2);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -12.0);
    EXPECT_EQ(data[1], -13.0);
    EXPECT_EQ(data[2], -22.0);
    EXPECT_EQ(data[3], -23.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -21.0);
    EXPECT_EQ(data[1], -22.0);
    EXPECT_EQ(data[2], -31.0);
    EXPECT_EQ(data[3], -32.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -22.0);
    EXPECT_EQ(data[1], -23.0);
    EXPECT_EQ(data[2], -32.0);
    EXPECT_EQ(data[3], -33.0);
  }
}

TEST(VariableSlice, slice_inner_minus_equals) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});

  var(Dim::X, 0) -= var(Dim::X, 1);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], -1.0);
  EXPECT_EQ(data[1], 2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, slice_outer_minus_equals) {
  auto var = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {1.0, 2.0, 3.0, 4.0});

  var(Dim::Y, 0) -= var(Dim::Y, 1);
  const auto data = var.get<const Data::Value>();
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], 3.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, nontrivial_slice_minus_equals) {
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                            {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source;
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                            {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source;
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                            {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source;
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}},
                                            {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source;
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableSlice, nontrivial_slice_minus_equals_slice) {
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>(
        {{Dim::Y, 2}, {Dim::X, 3}}, {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>(
        {{Dim::Y, 2}, {Dim::X, 3}}, {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>(
        {{Dim::Y, 2}, {Dim::X, 3}}, {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<Data::Value>(
        {{Dim::Y, 2}, {Dim::X, 3}}, {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.get<const Data::Value>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableSlice, slice_minus_lower_dimensional) {
  auto target = makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}});
  auto source = makeVariable<Data::Value>({Dim::X, 2}, {1.0, 2.0});
  EXPECT_EQ(target(Dim::Y, 1, 2).dimensions(),
            (Dimensions{{Dim::Y, 1}, {Dim::X, 2}}));

  target(Dim::Y, 1, 2) -= source;

  const auto data = target.get<const Data::Value>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, variable_copy_from_slice) {
  const auto source = makeVariable<Data::Value>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  Variable target1(source(Dim::X, 0, 2)(Dim::Y, 0, 2));
  EXPECT_EQ(target1.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target1.get<const Data::Value>(), {11, 12, 21, 22}));

  Variable target2(source(Dim::X, 1, 3)(Dim::Y, 0, 2));
  EXPECT_EQ(target2.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target2.get<const Data::Value>(), {12, 13, 22, 23}));

  Variable target3(source(Dim::X, 0, 2)(Dim::Y, 1, 3));
  EXPECT_EQ(target3.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target3.get<const Data::Value>(), {21, 22, 31, 32}));

  Variable target4(source(Dim::X, 1, 3)(Dim::Y, 1, 3));
  EXPECT_EQ(target4.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target4.get<const Data::Value>(), {22, 23, 32, 33}));
}

TEST(VariableSlice, variable_assign_from_slice) {
  auto target =
      makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const auto source = makeVariable<Data::Value>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = source(Dim::X, 0, 2)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get<const Data::Value>(), {11, 12, 21, 22}));

  target = source(Dim::X, 1, 3)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get<const Data::Value>(), {12, 13, 22, 23}));

  target = source(Dim::X, 0, 2)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get<const Data::Value>(), {21, 22, 31, 32}));

  target = source(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get<const Data::Value>(), {22, 23, 32, 33}));
}

TEST(VariableSlice, slice_assign_from_variable) {
  const auto source =
      makeVariable<Data::Value>({{Dim::Y, 2}, {Dim::X, 2}}, {11, 12, 21, 22});

  // We might want to mimick Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2).copyFrom(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.get<const Data::Value>(),
                       {11, 12, 0, 21, 22, 0, 0, 0, 0}));
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2).copyFrom(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.get<const Data::Value>(),
                       {0, 11, 12, 0, 21, 22, 0, 0, 0}));
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3).copyFrom(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.get<const Data::Value>(),
                       {0, 0, 0, 11, 12, 0, 21, 22, 0}));
  }
  {
    auto target = makeVariable<Data::Value>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3).copyFrom(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.get<const Data::Value>(),
                       {0, 0, 0, 0, 11, 12, 0, 21, 22}));
  }
}
