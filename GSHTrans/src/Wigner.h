#ifndef GSH_TRANS_WIGNER_GUARD_H
#define GSH_TRANS_WIGNER_GUARD_H

#include <Eigen/Core>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <execution>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <numbers>
#include <numeric>
#include <vector>

#include "Concepts.h"

namespace GSHTrans {

// Declare/Define some utility functions.
constexpr int MinusOneToPower(int m) { return m % 2 ? -1 : 1; }

template <std::floating_point Real>
Real WignerMinOrderAtUpperIndex(int, int, Real, Real, bool, bool);

template <std::floating_point Real>
Real WignerMaxOrderAtUpperIndex(int, int, Real, Real, bool, bool);

template <std::floating_point Real>
Real WignerMaxUpperIndexAtOrder(int, int, Real, Real, bool, bool);

template <std::floating_point Real>
Real WignerMinUpperIndexAtOrder(int, int, Real, Real, bool, bool);

/////////////////////////////////////////////////////////////////////////
//                         WignerN class                               //
/////////////////////////////////////////////////////////////////////////

template <std::floating_point Real, OrderRange Orders,
          Normalisation Norm = Ortho>
class WignerN {
 public:
  // Set member types.
  using value_type = Real;
  using iterator = std::vector<Real>::iterator;
  using const_iterator = std::vector<Real>::const_iterator;
  using difference_type = std::vector<Real>::difference_type;
  using size_type = std::vector<Real>::size_type;

  // Define internal range class.
  class Range {
   public:
    Range(iterator start, iterator finish) : _start{start}, _finish{finish} {}
    iterator begin() { return _start; }
    iterator end() { return _finish; }

   private:
    iterator _start;
    iterator _finish;
  };

  // Default constructor.
  WignerN() = default;

  // Copy constructor.
  WignerN(WignerN const &) = default;

  // Move constructor.
  WignerN(WignerN &&) = default;

  // Constructor for a single angle.
  WignerN(difference_type lMax, difference_type mMax, difference_type n,
          Real theta)
      : _lMax(lMax), _mMax(mMax), _n(n), _nTheta(1) {
    assert(_lMax >= 0);
    assert(_mMax >= 0 && _mMax <= _lMax);
    auto size = Count();
    _data = std::vector<Real>(size);
    ComputeValues(0, theta);
  }

  // Constructor for a range of angles.
  template <RealFloatingPointRange Range>
  WignerN(difference_type lMax, difference_type mMax, difference_type n,
          Range theta)
      : _lMax(lMax), _mMax(mMax), _n(n), _nTheta(theta.size()) {
    assert(_lMax >= 0);
    assert(_mMax >= 0 && _mMax <= _lMax);
    auto size = Count() * _nTheta;
    _data = std::vector<Real>(size);
    for (difference_type i = 0; i < _nTheta; i++) {
      ComputeValues(i, theta[i]);
    }
  }

  // Constructor given iterators to angles
  template <RealFloatingPointIterator Iterator>
  WignerN(difference_type lMax, difference_type mMax, difference_type n,
          Iterator thetaStart, Iterator thetaFinish)
      : _lMax(lMax),
        _mMax(mMax),
        _n(n),
        _nTheta(std::distance(thetaStart, thetaFinish)) {
    assert(_lMax >= 0);
    assert(_mMax >= 0 && _mMax <= _lMax);
    auto size = Count() * _nTheta;
    _data = std::vector<Real>(size);
    for (difference_type i = 0; i < _nTheta; i++) {
      ComputeValues(i, thetaStart[i]);
    }
  }

  // Copy assigment.
  WignerN &operator=(WignerN const &) = default;

  // Move assignment.
  WignerN &operator=(WignerN &&) = default;

  // Geters for basic data.
  auto MaxDegree() const { return _lMax; }
  auto MaxOrder() const { return _mMax; }
  auto UpperIndex() const { return _n; }
  auto NumberOfAngles() const { return _nTheta; }

  // Returns lowest order at given degree.
  auto StartingOrder(difference_type l) requires std::same_as<Orders, All> {
    assert(l <= _lMax && l >= std::abs(_n));
    return -std::min(l, _mMax);
  }
  auto StartingOrder(
      difference_type l) requires std::same_as<Orders, NonNegative> {
    assert(l <= _lMax && l >= std::abs(_n));
    return 0;
  }

  // Iterators to the data.
  iterator begin() { return _data.begin(); }
  const_iterator cbegin() const { return _data.cbegin(); }

  iterator end() { return _data.end(); }
  const_iterator cend() const { return _data.cend(); }

  iterator beginForDegree(difference_type l) {
    return std::next(begin(), Count(l - 1));
  }
  const_iterator cbeginForDegree(difference_type l) const {
    return std::next(cbegin(), Count(l - 1));
  }

  iterator end(difference_type l) { return std::next(begin(), Count(l)); }
  const_iterator cend(difference_type l) const {
    return std::next(cbegin(), Count(l));
  }

  iterator endForDegee(difference_type l) {
    return std::next(begin(), Count(l));
  }
  const_iterator cendForDegree(difference_type l) const {
    return std::next(cbegin(), Count(l));
  }

  iterator beginForAngle(difference_type i) {
    return std::next(begin(), i * Count());
  }
  const_iterator cbeginForAngle(difference_type i) const {
    return std::next(cbegin(), i * Count());
  }

  iterator endForAngle(difference_type i) {
    return std::next(begin(), (i + 1) * Count());
  }
  const_iterator cendForAngle(difference_type i) const {
    return std::next(cbegin(), (i + 1) * Count());
  }

  iterator beginForAngleAndDegree(difference_type i, difference_type l) {
    return std::next(beginForAngle(i), Count(l - 1));
  }
  const_iterator cbeginForAngleAndDegree(difference_type i,
                                         difference_type l) const {
    return std::next(cbeginForAngle(i), Count(l - 1));
  }

  iterator endForAngleAndDegree(difference_type i, difference_type l) {
    return std::next(beginForAngle(i), Count(l));
  }
  const_iterator cendForAngleAndDegree(difference_type i,
                                       difference_type l) const {
    return std::next(cbeginForAngle(i), Count(l));
  }

  // Ranges to the data.
  auto RangeForAngle(difference_type i) {
    assert(i >= 0 && i < _nTheta);
    return Range(beginForAngle(i), endForAngle(i));
  }

  auto RangeForAngleAndDegree(difference_type i, difference_type l) {
    assert(i >= 0 && i < _nTheta);
    return Range(beginForAngleAndDegree(i, l), endForAngleAndDegree(i, l));
  }

  // Returns the number of values at a given degree when
  // all orders are stored.
  constexpr size_type Count(
      difference_type l) const requires std::same_as<Orders, All> {
    auto nabs = std::abs(_n);
    if (l < nabs) return 0;
    if (l <= _mMax) return (l + 1) * (l + 1) - nabs * nabs;
    return (_mMax + 1) * (_mMax + 1) - nabs * nabs +
           (l - _mMax) * (2 * _mMax + 1);
  }

  // Returns the number of values at a given degree when
  // only non-negative orders are stored.
  constexpr size_type Count(
      difference_type l) const requires std::same_as<Orders, NonNegative> {
    auto nabs = std::abs(_n);
    if (l < nabs) return 0;
    if (l <= _mMax) return ((l + 1) * (l + 2)) / 2 - (nabs * (nabs + 1)) / 2;
    return ((_mMax + 1) * (_mMax + 2)) / 2 - (nabs * (nabs + 1)) / 2 +
           (l - _mMax) * (_mMax + 1);
  }

  // Returns total number of values.
  constexpr size_type Count() const { return Count(_lMax); }

  // Return value for given arguments.
  auto operator()(difference_type l,
                  difference_type m) const requires std::same_as<Orders, All> {
    assert(l >= std::abs(_n) && l <= _lMax);
    auto mMaxAbs = std::min(l, _mMax);
    assert(std::abs(m) <= mMaxAbs);
    return *std::next(cbeginForDegree(l), mMaxAbs + m);
  }

  auto operator()(difference_type l, difference_type m)
      const requires std::same_as<Orders, NonNegative> {
    assert(l >= std::abs(_n) && l <= _lMax);
    assert(0 <= m && m <= std::min(l, _mMax));
    return *std::next(cbeginForDegree(l), m);
  }

  auto operator()(difference_type i, difference_type l,
                  difference_type m) const requires std::same_as<Orders, All> {
    assert(i >= 0 && i < _nTheta);
    assert(l >= std::abs(_n) && l <= _lMax);
    auto mMaxAbs = std::min(l, _mMax);
    assert(std::abs(m) <= mMaxAbs);
    return *std::next(cbeginForAngleAndDegree(i, l), mMaxAbs + m);
  }

  auto operator()(difference_type i, difference_type l, difference_type m)
      const requires std::same_as<Orders, NonNegative> {
    assert(i >= 0 && i < _nTheta);
    assert(l >= std::abs(_n) && l <= _lMax);
    assert(0 <= m && m <= std::min(l, _mMax));
    return *std::next(cbeginForAngleAndDegree(i, l), m);
  }

 private:
  // Set the execution policy.
  static constexpr auto _policy = std::execution::seq;

  difference_type _lMax;    // Maximum degree.
  difference_type _mMax;    // Maximum order.
  difference_type _n;       // Upper index.
  difference_type _nTheta;  // Number of angles.

  // Vector to store the values.
  std::vector<Real> _data;

  // Storage passed as iterators.
  void ComputeValues(difference_type i, Real theta);
};

template <std::floating_point Real, OrderRange Orders, Normalisation Norm>
void WignerN<Real, Orders, Norm>::ComputeValues(difference_type i, Real theta) {
  // Pre-compute and store trigonometric terms.
  auto cos = std::cos(theta);
  auto logSinHalf = std::sin(0.5 * theta);
  auto logCosHalf = std::cos(0.5 * theta);
  auto atLeft = logSinHalf < std::numeric_limits<value_type>::min();
  auto atRight = logCosHalf < std::numeric_limits<value_type>::min();
  logSinHalf = atLeft ? static_cast<value_type>(0) : std::log(logSinHalf);
  logCosHalf = atRight ? static_cast<value_type>(0) : std::log(logCosHalf);

  // Pre-compute and store square roots and their inverses up to lMax + mMax.
  std::vector<value_type> sqInt(_lMax + _mMax + 1);
  std::transform(_policy, sqInt.begin(), sqInt.end(), sqInt.begin(),
                 [&](auto &x) {
                   return std::sqrt(static_cast<value_type>(&x - &sqInt[0]));
                 });
  std::vector<value_type> sqIntInv(_lMax + _mMax + 1);
  std::transform(
      _policy, sqInt.begin(), sqInt.end(), sqIntInv.begin(), [](auto x) {
        return x > static_cast<value_type>(0) ? 1 / x
                                              : static_cast<value_type>(0);
      });

  // Set the values for l == |n|
  const auto nabs = std::abs(_n);
  {
    auto l = nabs;
    auto mStart = StartingOrder(l);
    auto start = beginForAngleAndDegree(i, l);
    auto finish = endForAngleAndDegree(i, l);
    if (_n >= 0) {
      std::transform(_policy, start, finish, start, [&](auto &p) {
        auto m = mStart + std::distance(&*start, &p);
        return WignerMaxUpperIndexAtOrder(l, m, logSinHalf, logCosHalf, atLeft,
                                          atRight);
      });
    } else {
      std::transform(_policy, start, finish, start, [&](auto &p) {
        auto m = mStart + std::distance(&*start, &p);
        return WignerMinUpperIndexAtOrder(l, m, logSinHalf, logCosHalf, atLeft,
                                          atRight);
      });
    }
  }

  // Set the values for l == n+1 if needed.
  if (nabs < _lMax) {
    auto l = nabs + 1;
    auto mStart = StartingOrder(l);

    // Set iterators
    auto startMinusOne = beginForAngleAndDegree(i, l - 1);
    auto finishMinusOne = endForAngleAndDegree(i, l - 1);
    auto start = beginForAngleAndDegree(i, l);

    // Add in value at m == -l if needed.
    if constexpr (std::same_as<Orders, All>) {
      if (l <= _mMax) {
        *start++ = WignerMinOrderAtUpperIndex(l, _n, logSinHalf, logCosHalf,
                                              atLeft, atRight);
        // Update the starting order for recursion
        mStart += 1;
      }
    }

    // Add in interior orders using one-term recursion.
    {
      auto alpha = (2 * l - 1) * l * cos * sqIntInv[l + nabs];
      auto beta = (2 * l - 1) * sqIntInv[l + nabs];
      if (_n < 0) beta *= -1;
      std::transform(
          _policy, startMinusOne, finishMinusOne, start, [&](auto &minusOne) {
            auto m = mStart + std::distance(&*startMinusOne, &minusOne);
            auto f1 = (alpha - beta * m) * sqIntInv[l - m] * sqIntInv[l + m];
            return f1 * minusOne;
          });
    }

    // Add in value at m == l if needed
    if (l <= _mMax) {
      auto mStep = 2 * l - 1;
      if constexpr (std::same_as<Orders, NonNegative>) {
        mStep = l;
      }
      *std::next(start, mStep) = WignerMaxOrderAtUpperIndex(
          l, _n, logSinHalf, logCosHalf, atLeft, atRight);
    }
  }

  // Now do the remaining degrees.
  for (auto l = nabs + 2; l <= _lMax; l++) {
    // Starting order within two-term recursion
    auto mStart = StartingOrder(l);

    // Set iterators
    auto startMinusTwo = beginForAngleAndDegree(i, l - 2);
    auto finishMinusTwo = endForAngleAndDegree(i, l - 2);
    auto startMinusOne = beginForAngleAndDegree(i, l - 1);
    auto start = beginForAngleAndDegree(i, l);

    // Add in lower boundary terms if still growing.
    if constexpr (std::same_as<Orders, All>) {
      if (l <= _mMax) {
        // Add in the m == -l term.
        *start++ = WignerMinOrderAtUpperIndex(l, _n, logSinHalf, logCosHalf,
                                              atLeft, atRight);
        // Now do the m == -l+1 term using one-point recursion.
        {
          auto m = -l + 1;
          auto f1 = (2 * l - 1) * (l * (l - 1) * cos - m * _n) *
                    sqIntInv[l - _n] * sqIntInv[l + _n] * sqIntInv[l - m] *
                    sqIntInv[l + m] / static_cast<value_type>(l - 1);
          *start++ = f1 * (*startMinusOne++);
        }
        // Update the starting order for two-term recursion.
        mStart += 2;
      }

      // Add in the lower boundary term at the critical degree
      if (l == _mMax + 1) {
        auto m = -_mMax;
        auto f1 = (2 * l - 1) * (l * (l - 1) * cos - m * _n) *
                  sqIntInv[l - _n] * sqIntInv[l + _n] * sqIntInv[l - m] *
                  sqIntInv[l + m] / static_cast<value_type>(l - 1);
        *start++ = f1 * (*startMinusOne++);
        // Update the starting order for two-term recursion.
        mStart += 1;
      }
    }

    // Apply two-term recusion for the interior orders.
    {
      auto alpha = (2 * l - 1) * l * cos * sqIntInv[l - _n] * sqIntInv[l + _n];
      auto beta = (2 * l - 1) * _n * sqIntInv[l - _n] * sqIntInv[l + _n] /
                  static_cast<value_type>(l - 1);
      auto gamma = l * sqInt[l - 1 - _n] * sqInt[l - 1 + _n] *
                   sqIntInv[l - _n] * sqIntInv[l + _n] /
                   static_cast<value_type>(l - 1);
      std::transform(
          _policy, startMinusTwo, finishMinusTwo, startMinusOne, start,
          [&](auto &minusTwo, auto &minusOne) {
            auto m = mStart + std::distance(&*startMinusTwo, &minusTwo);
            auto denom = sqIntInv[l - m] * sqIntInv[l + m];
            auto f1 = (alpha - beta * m) * denom;
            auto f2 = gamma * sqInt[l - 1 - m] * sqInt[l - 1 + m] * denom;
            return f1 * minusOne - f2 * minusTwo;
          });
    }

    // Add in the upper boundary terms if still growing.
    if (l <= _mMax) {
      // Update the iterator
      auto mStep = 2 * l - 3;
      if constexpr (std::same_as<Orders, NonNegative>) {
        mStep = l - 1;
      }
      startMinusOne = std::next(startMinusOne, mStep);
      start = std::next(start, mStep);
      // Add in m == l - 1 term using one-point recursion.
      {
        auto m = l - 1;
        auto f1 = (2 * l - 1) * (l * (l - 1) * cos - m * _n) *
                  sqIntInv[l - _n] * sqIntInv[l + _n] * sqIntInv[l - m] *
                  sqIntInv[l + m] / static_cast<value_type>(l - 1);
        *start++ = f1 * (*startMinusOne++);
      }
      // Now do m == l.
      *start++ = WignerMaxOrderAtUpperIndex(l, _n, logSinHalf, logCosHalf,
                                            atLeft, atRight);
    }

    // Add in the upper boundary term at the crtiical degree.
    if (l == _mMax + 1) {
      // Update the iterators.
      auto mStep = 2 * _mMax - 1;
      if constexpr (std::same_as<Orders, NonNegative>) {
        mStep = _mMax;
      }
      startMinusOne = std::next(startMinusOne, mStep);
      start = std::next(start, mStep);
      auto m = _mMax;
      auto f1 = (2 * l - 1) * (l * (l - 1) * cos - m * _n) * sqIntInv[l - _n] *
                sqIntInv[l + _n] * sqIntInv[l - m] * sqIntInv[l + m] /
                static_cast<value_type>(l - 1);
      *start++ = f1 * (*startMinusOne++);
    }
  }

  if constexpr (std::same_as<Norm, Ortho>) {
    for (auto l = 0; l <= _lMax; l++) {
      auto start = beginForAngleAndDegree(i, l);
      auto finish = endForAngleAndDegree(i, l);
      std::transform(start, finish, start, [l](auto p) {
        return 0.5 * std::sqrt(static_cast<value_type>(2 * l + 1)) *
               std::numbers::inv_sqrtpi_v<value_type> * p;
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//      Simple function to compute Wigner values using +/- recursion in m     //
////////////////////////////////////////////////////////////////////////////////

template <std::floating_point Real, Normalisation Norm = Ortho>
Real Wigner(int l, int m, int n, Real theta) {
  // Check the inputs.
  assert(l >= 0);
  assert(std::abs(m) <= l);
  assert(std::abs(n) <= l);

  // Deal with l = 0 separately.
  if (l == 0) {
    if constexpr (std::same_as<Norm, Ortho>) {
      return 0.5 * std::numbers::inv_sqrtpi_v<Real>;
    } else {
      return 1;
    }
  }

  // Pre-compute some trigonometric terms.
  auto logSinHalf = std::sin(0.5 * theta);
  auto logCosHalf = std::cos(0.5 * theta);
  auto atLeft = logSinHalf < std::numeric_limits<Real>::min();
  auto atRight = logCosHalf < std::numeric_limits<Real>::min();

  // Deal with values at the end points
  if (atLeft) {
    if (n == -l) {
      if constexpr (std::same_as<Norm, Ortho>) {
        return 0.5 * std::sqrt(static_cast<Real>(2 * l + 1)) *
               std::numbers::inv_sqrtpi_v<Real>;
      } else {
        return 1;
      }
    } else {
      return 0;
    }
  }

  if (atRight) {
    if (n == l) {
      if constexpr (std::same_as<Norm, Ortho>) {
        return MinusOneToPower(l + n) * 0.5 *
               std::sqrt(static_cast<Real>(2 * l + 1)) *
               std::numbers::inv_sqrtpi_v<Real>;
      } else {
        return MinusOneToPower(l + n);
      }
    } else {
      return 0;
    }
  }

  // Compute remaining trigonometric terms
  logSinHalf = std::log(logSinHalf);
  logCosHalf = std::log(logCosHalf);
  auto cosec = static_cast<Real>(1) / std::sin(theta);
  auto cot = std::cos(theta) * cosec;

  // Pre-compute and store square roots and their inverses up to 2*l+1.
  std::vector<Real> sqInt(2 * l + 1);
  std::transform(sqInt.begin(), sqInt.end(), sqInt.begin(), [&](auto &x) {
    return std::sqrt(static_cast<Real>(&x - &sqInt[0]));
  });
  std::vector<Real> sqIntInv(2 * l + 1);
  std::transform(sqInt.begin(), sqInt.end(), sqIntInv.begin(), [](auto x) {
    return x > static_cast<Real>(0) ? 1 / x : static_cast<Real>(0);
  });

  // Compute the optimal meeting point for the recursion.
  int mOpt = n * std::cos(theta);

  // Apply upward recursion from m = -l.
  if (m <= mOpt) {
    Real minusOne = 0;
    Real current =
        WignerMinOrderAtUpperIndex(l, n, logSinHalf, logCosHalf, false, false);
    for (int mp = -l; mp < m; mp++) {
      current = 2 * (n * cosec - mp * cot) * sqIntInv[l - mp] *
                    sqIntInv[l + mp + 1] * current -
                sqInt[l + mp] * sqInt[l - mp + 1] * sqIntInv[l - mp] *
                    sqIntInv[l + mp + 1] * minusOne;
      minusOne = current;
    }
    if constexpr (std::same_as<Norm, Ortho>) {
      return 0.5 * std::sqrt(static_cast<Real>(2 * l + 1)) *
             std::numbers::inv_sqrtpi_v<Real> * current;
    } else {
      return current;
    }
  }

  // Apply downward recursion from m = l.
  if (m > mOpt) {
    Real plusOne = 0;
    Real current =
        WignerMaxOrderAtUpperIndex(l, n, logSinHalf, logCosHalf, false, false);
    for (int mp = l; mp > m; mp--) {
      current = 2 * (n * cosec - mp * cot) * sqIntInv[l + mp] *
                    sqIntInv[l - mp + 1] * current -
                sqInt[l - mp] * sqInt[l + mp + 1] * sqIntInv[l + mp] *
                    sqIntInv[l - mp + 1] * plusOne;
      plusOne = current;
    }
    if constexpr (std::same_as<Norm, Ortho>) {
      return 0.5 * std::sqrt(static_cast<Real>(2 * l + 1)) *
             std::numbers::inv_sqrtpi_v<Real> * current;
    } else {
      return current;
    }
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////
//                           Utility functions                         //
/////////////////////////////////////////////////////////////////////////

template <std::floating_point Real>
Real WignerMinOrderAtUpperIndex(int l, int n, Real logSinHalf, Real logCosHalf,
                                bool atLeft, bool atRight) {
  // Check the inputs.
  assert(l >= 0);
  assert(std::abs(n) <= l);

  // Deal with l == 0 case
  if (l == 0) return static_cast<Real>(1);

  // Deal with special case at the left boundary.
  if (atLeft) {
    return n == -l ? static_cast<Real>(1) : static_cast<Real>(0);
  }

  // Deal with special case at the right boundary.
  if (atRight) {
    return n == l ? static_cast<Real>(1) : static_cast<Real>(0);
  }

  // Deal with the general case.
  auto Fl = static_cast<Real>(l);
  auto Fn = static_cast<Real>(n);
  using std::exp;
  using std::lgamma;
  return exp(
      static_cast<Real>(0.5) *
          (lgamma(2 * Fl + 1) - lgamma(Fl - Fn + 1) - lgamma(Fl + Fn + 1)) +
      (Fl + Fn) * logSinHalf + (Fl - Fn) * logCosHalf);
}

template <std::floating_point Real>
Real WignerMaxOrderAtUpperIndex(int l, int n, Real logSinHalf, Real logCosHalf,
                                bool atLeft, bool atRight) {
  return MinusOneToPower(n + l) * WignerMinOrderAtUpperIndex(l, -n, logSinHalf,
                                                             logCosHalf, atLeft,
                                                             atRight);
}

template <std::floating_point Real>
Real WignerMinUpperIndexAtOrder(int l, int m, Real logSinHalf, Real logCosHalf,
                                bool atLeft, bool atRight) {
  return WignerMaxOrderAtUpperIndex(l, -m, logSinHalf, logCosHalf, atLeft,
                                    atRight);
}

template <std::floating_point Real>
Real WignerMaxUpperIndexAtOrder(int l, int m, Real logSinHalf, Real logCosHalf,
                                bool atLeft, bool atRight) {
  return WignerMinOrderAtUpperIndex(l, -m, logSinHalf, logCosHalf, atLeft,
                                    atRight);
}

}  // namespace GSHTrans

#endif  // GSH_TRANS_WIGNER_GUARD_H
