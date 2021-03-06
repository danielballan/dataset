/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include <memory>
#include <utility>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <gsl/gsl_util>
#include <gsl/span>

#include "dimension.h"
#include "except.h"

/// Dimensions are accessed very frequently, so packing everything into a single
/// (64 Byte) cacheline should be advantageous.
/// We should follow the numpy convention: First dimension is outer dimension,
/// last dimension is inner dimension, for now we do not.
class Dimensions {
public:
  Dimensions() noexcept {}
  Dimensions(const Dim dim, const gsl::index size)
      : Dimensions({{dim, size}}) {}
  Dimensions(const std::initializer_list<std::pair<Dim, gsl::index>> dims) {
    // TODO Check for duplicate dimension.
    m_ndim = dims.size();
    if (m_ndim > 6)
      throw std::runtime_error("At most 6 dimensions are supported.");
    auto dim = dims.begin();
    for (gsl::index i = 0; i < m_ndim; ++i, ++dim) {
      m_dims[i] = dim->first;
      if (m_dims[i] == Dim::Invalid)
        throw std::runtime_error("Dim::Invalid is not a valid dimension.");
      m_shape[i] = dim->second;
      if (m_shape[i] < 0)
        throw std::runtime_error("Dimension extent cannot be negative.");
    }
  }

  bool operator==(const Dimensions &other) const noexcept {
    if (m_ndim != other.m_ndim)
      return false;
    for (int32_t i = 0; i < 6; ++i) {
      if (m_shape[i] != other.m_shape[i])
        return false;
      if (m_dims[i] != other.m_dims[i])
        return false;
    }
    return true;
  }
  bool operator!=(const Dimensions &other) const noexcept {
    return !(*this == other);
  }

  bool empty() const noexcept { return m_ndim == 0; }

  int32_t ndim() const noexcept { return m_ndim; }
  // TODO Remove in favor of the new ndim?
  int32_t count() const noexcept { return m_ndim; }

  gsl::index volume() const noexcept {
    gsl::index volume{1};
    for (int32_t dim = 0; dim < ndim(); ++dim)
      volume *= m_shape[dim];
    return volume;
  }

  gsl::span<const gsl::index> shape() const noexcept {
    return {m_shape, m_shape + m_ndim};
  }

  gsl::span<const Dim> labels() const noexcept {
    return {m_dims, m_dims + m_ndim};
  }

  gsl::index operator[](const Dim dim) const {
    for (int32_t i = 0; i < 6; ++i)
      if (m_dims[i] == dim)
        return m_shape[i];
    throw dataset::except::DimensionNotFoundError(*this, dim);
  }

  gsl::index &operator[](const Dim dim) {
    for (int32_t i = 0; i < 6; ++i)
      if (m_dims[i] == dim)
        return m_shape[i];
    throw dataset::except::DimensionNotFoundError(*this, dim);
  }

  bool contains(const Dim dim) const noexcept {
    for (int32_t i = 0; i < ndim(); ++i)
      if (m_dims[i] == dim)
        return true;
    return false;
  }

  bool contains(const Dimensions &other) const;

  bool isContiguousIn(const Dimensions &parent) const;

  Dimension label(const gsl::index i) const;
  void relabel(const gsl::index i, const Dimension label) { m_dims[i] = label; }
  gsl::index size(const gsl::index i) const;
  gsl::index size(const Dimension label) const;
  gsl::index offset(const Dimension label) const;
  void resize(const Dimension label, const gsl::index size);
  void resize(const gsl::index i, const gsl::index size);
  void erase(const Dimension label);

  void add(const Dimension label, const gsl::index size);

  // auto begin() const { return m_dims.begin(); }
  // auto end() const { return m_dims.end(); }

  gsl::index index(const Dimension label) const;

private:
  // This is 56 Byte, or would be 40 Byte for small size 1.
  // boost::container::small_vector<std::pair<Dimension, gsl::index>, 2> m_dims;
  // Support at most 6 dimensions, should be sufficient?
  // 6*8 Byte = 48 Byte
  gsl::index m_shape[6]{-1, -1, -1, -1, -1, -1};
  int32_t m_ndim{0};
  Dim m_dims[6]{Dim::Invalid, Dim::Invalid, Dim::Invalid,
                Dim::Invalid, Dim::Invalid, Dim::Invalid};
};

Dimensions concatenate(const Dimension dim, const Dimensions &dims1,
                       const Dimensions &dims2);

#endif // DIMENSIONS_H
