/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DATASET_H
#define DATASET_H

#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <gsl/gsl_util>

#include "dimension.h"
#include "tags.h"
#include "variable.h"

class Dataset;
namespace detail {
template <class Tag> class VariableView;
template <class Tag> VariableView<Tag> getCoord(Dataset &, const Tag);
template <class Tag>
VariableView<Tag> getData(Dataset &,
                          const std::pair<const Tag, const std::string> &);
template <class Data> class Access;
} // namespace detail

template <class T> class Slice;

class Dataset {
public:
  Dataset() = default;
  // Allowing implicit construction from views facilities calling functions that
  // do not explicitly support views. It is open for discussion whether this is
  // a good idea or not.
  Dataset(const Slice<const Dataset> &view);
  Dataset(const Slice<Dataset> &view);

  gsl::index size() const { return m_variables.size(); }
  const Variable &operator[](const gsl::index i) const {
    return m_variables[i];
  }
  // WARNING: This returns `const Variable &` ON PURPOSE. We do not provide
  // non-const access to Variable since it could break the dataset, e.g., by
  // assigning a variable with a different shape. Nevetheless we need the
  // non-const overload to avoid compiler warnings about ambiguous overloads
  // with the std::string version.
  const Variable &operator[](const gsl::index i) { return m_variables[i]; }
  Slice<const Dataset> operator[](const std::string &name) const;
  Slice<Dataset> operator[](const std::string &name);
  Slice<const Dataset> operator()(const Dim dim, const gsl::index begin,
                                  const gsl::index end = -1) const;
  Slice<Dataset> operator()(const Dim dim, const gsl::index begin,
                            const gsl::index end = -1);

  auto begin() const { return m_variables.begin(); }
  auto end() const { return m_variables.end(); }

  void insert(Variable variable);

  template <class Tag, class... Args>
  void insert(const Dimensions &dimensions, Args &&... args) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    insert(std::move(a));
  }

  template <class Tag, class... Args>
  void insert(const std::string &name, const Dimensions &dimensions,
              Args &&... args) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a =
        makeVariable<Tag>(std::move(dimensions), std::forward<Args>(args)...);
    a.setName(name);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const Dimensions &dimensions, std::initializer_list<T> values) {
    static_assert(is_coord<Tag>, "Non-coordinate variable must have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    insert(std::move(a));
  }

  template <class Tag, class T>
  void insert(const std::string &name, const Dimensions &dimensions,
              std::initializer_list<T> values) {
    static_assert(!is_coord<Tag>, "Coordinate variable cannot have a name.");
    auto a = makeVariable<Tag>(std::move(dimensions), values);
    a.setName(name);
    insert(std::move(a));
  }

  bool contains(const Tag tag, const std::string &name = "") const;
  void erase(const Tag tag, const std::string &name = "");

  template <class Tag> void erase() {
    const auto it = m_variables.begin() + findUnique(tag<Tag>);
    const auto dims = it->dimensions();
    m_variables.erase(it);
    for (const auto dim : dims.labels()) {
      bool found = false;
      for (const auto &var : m_variables)
        if (var.dimensions().contains(dim))
          found = true;
      if (!found)
        m_dimensions.erase(dim);
    }
  }

  Dataset extract(const std::string &name);

  void merge(const Dataset &other) {
    for (const auto &var : other)
      insert(var);
  }

  template <class Tag> auto get() const {
    return m_variables[findUnique(tag<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  template <class Tag> auto get() {
    return m_variables[findUnique(tag<Tag>)].template get<Tag>();
  }

  template <class Tag> auto get(const std::string &name) {
    return m_variables[find(tag_id<Tag>, name)].template get<Tag>();
  }

  const Dimensions &dimensions() const { return m_dimensions; }

  template <class Tag> const Dimensions &dimensions() const {
    return m_variables[findUnique(tag<Tag>)].dimensions();
  }

  template <class Tag>
  const Dimensions &dimensions(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].dimensions();
  }

  template <class Tag> const Unit &unit() const {
    return m_variables[findUnique(tag<Tag>)].unit();
  }

  template <class Tag> const Unit &unit(const std::string &name) const {
    return m_variables[find(tag_id<Tag>, name)].unit();
  }

  gsl::index find(const uint16_t id, const std::string &name) const;
  gsl::index findUnique(const Tag tag) const;

  bool operator==(const Dataset &other) const;
  template <class T> Dataset &operator+=(const T &other);
  template <class T> Dataset &operator-=(const T &other);
  template <class T> Dataset &operator*=(const T &other);
  void setSlice(const Dataset &slice, const Dimension dim,
                const gsl::index index);

private:
  template <class Data> friend class detail::Access;
  // This is private such that name and dimensions of variables cannot be
  // modified in a way that would break the dataset.
  Variable &get(gsl::index i) { return m_variables[i]; }

  void mergeDimensions(const Dimensions &dims, const Dim coordDim);

  // TODO These dimensions do not imply any ordering, should use another class
  // in place of `Dimensions`, which *does* imply an order.
  Dimensions m_dimensions;
  boost::container::small_vector<Variable, 4> m_variables;
};

template <class T> gsl::index count(const T &dataset, const uint16_t id) {
  gsl::index n = 0;
  for (const auto &item : dataset)
    if (item.type() == id)
      ++n;
  return n;
}

template <class T>
gsl::index count(const T &dataset, const uint16_t id, const std::string &name) {
  gsl::index n = 0;
  for (const auto &item : dataset)
    if (item.type() == id && item.name() == name)
      ++n;
  return n;
}

template <class Value>
class dataset_slice_iterator
    : public boost::iterator_facade<
          dataset_slice_iterator<Value>,
          VariableSlice<std::conditional_t<std::is_const<Value>::value,
                                           const Variable, Variable>>,
          boost::random_access_traversal_tag,
          VariableSlice<std::conditional_t<std::is_const<Value>::value,
                                           const Variable, Variable>>> {
public:
  dataset_slice_iterator(
      Value &dataset, const std::vector<gsl::index> &indices,
      const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &slices,
      const gsl::index index)
      : m_dataset(dataset), m_indices(indices), m_slices(slices),
        m_index(index) {}

private:
  friend class boost::iterator_core_access;

  bool equal(const dataset_slice_iterator &other) const {
    return m_index == other.m_index;
  }
  void increment() { ++m_index; }
  VariableSlice<
      std::conditional_t<std::is_const<Value>::value, const Variable, Variable>>
  dereference() const;
  void decrement() { --m_index; }
  void advance(int64_t delta) { m_index += delta; }
  int64_t distance_to(const dataset_slice_iterator &other) const {
    return other.m_index - m_index;
  }

  // TODO Just reference Slice instead of all its contents here.
  Value &m_dataset;
  const std::vector<gsl::index> &m_indices;
  const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &m_slices;
  gsl::index m_index;
};

// T can be Dataset or Slice.
template <class T>
gsl::index find(const T &dataset, const uint16_t id, const std::string &name) {
  for (gsl::index i = 0; i < dataset.size(); ++i)
    if (dataset[i].type() == id && dataset[i].name() == name)
      return i;
  throw std::runtime_error("Dataset does not contain such a variable.");
}

template <class Base> class SliceMutableMixin {};

template <> class SliceMutableMixin<Slice<Dataset>> {
public:
  template <class T> Slice<Dataset> operator+=(const T &other);
  template <class T> Slice<Dataset> operator-=(const T &other);
  template <class T> Slice<Dataset> operator*=(const T &other);

private:
  VariableSlice<Variable> get(const gsl::index i);
  dataset_slice_iterator<Dataset> mutableBegin() const;
  dataset_slice_iterator<Dataset> mutableEnd() const;

  friend class detail::Access<Slice<Dataset>>;
  const Slice<Dataset> &base() const;
  Slice<Dataset> &base();
};

namespace detail {
template <class Var>
VariableSlice<Var>
makeSlice(Var &variable,
          const std::vector<std::tuple<Dim, gsl::index, gsl::index>> &slices) {
  auto slice = VariableSlice<Var>(variable);
  for (const auto &s : slices) {
    if (variable.dimensions().contains(std::get<Dim>(s)))
      slice = slice(std::get<0>(s), std::get<1>(s), std::get<2>(s));
  }
  return slice;
}
} // namespace detail

// D is either Dataset or const Dataset.
template <class D> class Slice : public SliceMutableMixin<Slice<D>> {
public:
  Slice(D &dataset) : m_dataset(dataset) {
    // Select everything.
    for (gsl::index i = 0; i < dataset.size(); ++i)
      m_indices.push_back(i);
  }

  Slice(D &dataset, const std::string &select) : m_dataset(dataset) {
    for (gsl::index i = 0; i < dataset.size(); ++i) {
      const auto &var = dataset[i];
      if (var.isCoord() || var.name() == select)
        m_indices.push_back(i);
    }
  }

  Slice operator()(const Dim dim, const gsl::index begin,
                   const gsl::index end = -1) const {
    auto slice(*this);
    for (auto &s : slice.m_slices) {
      if (std::get<Dim>(s) == dim) {
        std::get<1>(s) = begin;
        std::get<2>(s) = end;
        return slice;
      }
    }
    slice.m_slices.emplace_back(dim, begin, end);
    if (end == -1) {
      for (auto it = slice.m_indices.begin(); it != slice.m_indices.end();) {
        // TODO Should all coordinates with matching dimension be removed, or
        // only dimension-coordinates?
        if (coordDimension[m_dataset[*it].type()] == dim)
          it = slice.m_indices.erase(it);
        else
          ++it;
      }
    }
    return slice;
  }

  bool contains(const Tag tag, const std::string &name = "") const;

  const Dataset &dataset() const { return m_dataset; }

  std::vector<std::tuple<Dim, gsl::index>> dimensions() {
    std::vector<std::tuple<Dim, gsl::index>> dims;
    for (gsl::index i = 0; i < m_dataset.dimensions().count(); ++i) {
      const Dim dim = m_dataset.dimensions().label(i);
      gsl::index size = m_dataset.dimensions().size(i);
      for (const auto &slice : m_slices)
        if (std::get<Dim>(slice) == dim) {
          if (std::get<2>(slice) == -1)
            size = -1;
          else
            size = std::get<2>(slice) - std::get<1>(slice);
        }
      dims.emplace_back(dim, size);
    }
    return dims;
  }

  gsl::index size() const { return m_indices.size(); }

  VariableSlice<const Variable> operator[](const gsl::index i) const {
    return detail::makeSlice(m_dataset[m_indices[i]], m_slices);
  }

  dataset_slice_iterator<const Dataset> begin() const {
    return {dataset(), m_indices, m_slices, 0};
  }
  dataset_slice_iterator<const Dataset> end() const {
    return {dataset(), m_indices, m_slices,
            static_cast<gsl::index>(m_indices.size())};
  }

  bool operator==(const Slice<D> &other) const {
    if ((m_dataset == other.m_dataset) && (m_indices == other.m_indices) &&
        (m_slices == other.m_slices))
      return true;
    return std::equal(begin(), end(), other.begin(), other.end());
  }

private:
  friend class SliceMutableMixin<Slice<D>>;

  D &m_dataset;
  std::vector<gsl::index> m_indices;
  std::vector<std::tuple<Dim, gsl::index, gsl::index>> m_slices;
};

namespace detail {
template <class Data> class Access;

template <> class Access<Dataset> {
public:
  Access(Dataset &dataset) : m_data(dataset) {}

  auto begin() const { return m_data.m_variables.begin(); }
  auto end() const { return m_data.m_variables.end(); }
  Variable &operator[](const gsl::index i) const {
    return m_data.m_variables[i];
  }

private:
  Dataset &m_data;
};

template <> class Access<Slice<Dataset>> {
public:
  Access(Slice<Dataset> &dataset) : m_data(dataset) {}

  auto begin() const { return m_data.mutableBegin(); }
  auto end() const { return m_data.mutableEnd(); }
  VariableSlice<Variable> operator[](const gsl::index i) const {
    return VariableSlice<Variable>(m_data.get(i));
  }

private:
  Slice<Dataset> &m_data;
};

inline const Dataset &makeAccess(const Dataset &dataset) { return dataset; }
inline auto makeAccess(Dataset &dataset) { return Access<Dataset>(dataset); }
inline auto makeAccess(Slice<Dataset> &dataset) {
  return Access<Slice<Dataset>>(dataset);
}

} // namespace detail

Dataset operator+(Dataset a, const Dataset &b);
Dataset operator-(Dataset a, const Dataset &b);
Dataset operator*(Dataset a, const Dataset &b);
Dataset slice(const Dataset &d, const Dimension dim, const gsl::index index);
Dataset slice(const Dataset &d, const Dimension dim, const gsl::index begin,
              const gsl::index end);
std::vector<Dataset> split(const Dataset &d, const Dim dim,
                           const std::vector<gsl::index> &indices);
Dataset concatenate(const Dataset &d1, const Dataset &d2, const Dimension dim);
// Not implemented
Dataset convert(const Dataset &d, const Dimension from, const Dimension to);
// Not verified, likely wrong in some cases
Dataset rebin(const Dataset &d, const Variable &newCoord);

Dataset sort(const Dataset &d, const Tag t, const std::string &name = "");
// Note: Can provide stable_sort for sorting by multiple columns, e.g., for a
// QTableView.

Dataset filter(const Dataset &d, const Variable &select);

#endif // DATASET_H
