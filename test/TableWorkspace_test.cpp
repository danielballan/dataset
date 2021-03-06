/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset_view.h"

// Quick and dirty conversion to strings, should probably be part of our library
// of basic routines.
std::vector<std::string> asStrings(const Variable &variable) {
  std::vector<std::string> strings;
  if (variable.valueTypeIs<Coord::RowLabel>())
    for (const auto &value : variable.get<const Coord::RowLabel>())
      strings.emplace_back(value);
  if (variable.valueTypeIs<Data::Value>())
    for (const auto &value : variable.get<const Data::Value>())
      strings.emplace_back(std::to_string(value));
  else if (variable.valueTypeIs<Data::String>())
    for (const auto &value : variable.get<const Data::String>())
      strings.emplace_back(value);
  return strings;
}

TEST(TableWorkspace, basics) {
  Dataset table;
  table.insert<Coord::RowLabel>({Dimension::Row, 3},
                                Vector<std::string>{"a", "b", "c"});
  table.insert<Data::Value>("Data", {Dimension::Row, 3}, {1.0, -2.0, 3.0});
  table.insert<Data::String>("Comment", {Dimension::Row, 3}, 3);

  // Modify table with know columns.
  DatasetView<const Data::Value, Data::String> view(table);
  for (auto &item : view)
    if (item.value() < 0.0)
      item.get<Data::String>() = "why is this negative?";

  // Get string representation of arbitrary table, e.g., for visualization.
  EXPECT_EQ(asStrings(table[0]), std::vector<std::string>({"a", "b", "c"}));
  EXPECT_EQ(asStrings(table[1]),
            std::vector<std::string>({"1.000000", "-2.000000", "3.000000"}));
  EXPECT_EQ(asStrings(table[2]),
            std::vector<std::string>({"", "why is this negative?", ""}));

  // Standard shape operations provide basic things required for tables:
  auto mergedTable = concatenate(table, table, Dimension::Row);
  auto row = slice(table, Dimension::Row, 1);
  EXPECT_EQ(row.get<const Coord::RowLabel>()[0], "b");

  // Slice a range to obtain a new table with a subset of rows.
  auto rows = slice(mergedTable, Dimension::Row, 1, 4);
  ASSERT_EQ(rows.get<const Coord::RowLabel>().size(), 3);
  EXPECT_EQ(rows.get<const Coord::RowLabel>()[0], "b");
  EXPECT_EQ(rows.get<const Coord::RowLabel>()[1], "c");
  EXPECT_EQ(rows.get<const Coord::RowLabel>()[2], "a");

  // Can sort by arbitrary column.
  auto sortedTable = sort(table, tag<Data::Value>, "Data");
  EXPECT_EQ(asStrings(sortedTable[0]),
            std::vector<std::string>({"b", "a", "c"}));
  EXPECT_EQ(asStrings(sortedTable[1]),
            std::vector<std::string>({"-2.000000", "1.000000", "3.000000"}));
  EXPECT_EQ(asStrings(sortedTable[2]),
            std::vector<std::string>({"why is this negative?", "", ""}));

  // Split (opposite of concatenate).
  auto parts = split(mergedTable, Dimension::Row, {3});
  ASSERT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0], table);
  EXPECT_EQ(parts[1], table);

  // Remove rows from the middle of a table.
  auto recombined = concatenate(mergedTable(Dim::Row, 0, 2),
                                mergedTable(Dim::Row, 4, 6), Dim::Row);
  EXPECT_EQ(asStrings(recombined[0]),
            std::vector<std::string>({"a", "b", "b", "c"}));

  // Other basics (to be implemented): cut/truncate/chop/extract (naming
  // unclear), filter, etc.
}
