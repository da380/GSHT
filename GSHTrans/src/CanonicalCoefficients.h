#ifndef GSH_TRANS_CANONICAL_COEFFICIENTS_GUARD_H
#define GSH_TRANS_CANONICAL_COEFFICIENTS_GUARD_H

#include <FFTWpp/All>
#include <algorithm>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/tuple/tuple.hpp>
#include <complex>
#include <functional>
#include <iostream>
#include <ranges>
#include <type_traits>

#include "Concepts.h"
#include "Indexing.h"

namespace GSHTrans {

/*

//----------------------------------------------------//
//        Forward declarations of some classes        //
//----------------------------------------------------//

template <typename Derived>
class CanonicalCoefficientBase;

template <typename GSHGrid, RealOrComplexValued ValueType>
class CanonicalCoefficient;

template <typename GSHGrid>
using RealCanonicalCoefficient = CanonicalCoefficient<GSHGrid, RealValued>;

template <typename GSHGrid>
using ComplexCanonicalCoefficient =
    CanonicalCoefficient<GSHGrid, ComplexValued>;

template <typename GSHGrid, RealOrComplexValued ValueType,
          std::ranges::common_range View>
requires std::same_as<RemoveComplex<std::ranges::range_value_t<View>>,
                      typename GSHGrid::real_type>
class CanonicalCoefficientView;

template <typename GSHGrid, std::ranges::common_range View>
requires std::same_as<RemoveComplex<std::ranges::range_value_t<View>>,
                      typename GSHGrid::real_type>
using RealCanonicalCoefficientView =
    CanonicalCoefficientView<GSHGrid, RealValued, View>;

template <typename GSHGrid, std::ranges::common_range View>
requires std::same_as<RemoveComplex<std::ranges::range_value_t<View>>,
                      typename GSHGrid::real_type>
using ComplexCanonicalCoefficientView =
    CanonicalCoefficientView<GSHGrid, ComplexValued, View>;

//----------------------------------------------------//
//       Base class for canonical coefficients        //
//----------------------------------------------------//

template <typename Derived>
class CanonicalCoefficientBase {
  using Int = std::ptrdiff_t;

 public:
  auto DataView() { return GetDerived()._DataView(); }
  auto& Grid() const { return GetDerived()._Grid(); }

  auto begin() { return DataView().begin(); }
  auto end() { return DataView().end(); }
  auto size() { return DataView().size(); }

  auto MaxDegree() { return GetDerived()._MaxDegree(); }
  auto UpperIndex() { return GetDerived()._UpperIndex(); }

  auto Indices() {
    return GSHIndices<typename Derived::MRange>(MaxDegree(), MaxDegree(),
                                                UpperIndex());
  }

  auto MinDegree() { return Indices().MinDegree(); }
  auto Degrees() { return Indices().Degrees(); }

  auto MinOrder(Int l) { return Indices().MinOrder(l); }
  auto MaxOrder(Int l) { return Indices().MaxOrder(l); }
  auto Orders(Int l) { return Indices().Orders(l); }

  auto operator[](Int i) {
    assert(i >= 0 && i < size());
    return DataView()[i];
  }

  auto operator()(Int l, Int m) {
    auto i = Indices()(l, m);
    return operator[](i);
  }

 private:
  auto& GetDerived() { return static_cast<Derived&>(*this); }
  auto& GetDerived() const { return static_cast<const Derived&>(*this); }
};

//----------------------------------------------------//
//             Canonical coefficients class           //
//----------------------------------------------------//

template <typename GSHGrid, RealOrComplexValued ValueType>
class CanonicalCoefficient : public CanonicalCoefficientBase<
                                 CanonicalCoefficient<GSHGrid, ValueType>> {
  using Int = std::ptrdiff_t;
  using Real = typename GSHGrid::real_type;
  using Complex = typename GSHGrid::complex_type;
  using Vector = FFTWpp::vector<Complex>;

 public:
  using value_type = Complex;
  using coefficient_type = ValueType;
  using MRange =
      std::conditional_t<std::same_as<ValueType, RealValued>, NonNegative, All>;

  CanonicalCoefficient(Int lMax, Int n, GSHGrid& grid)
      : _lMax{lMax},
        _n{n},
        _grid{grid},
        _data{Vector(GSHIndices<MRange>(_lMax, _lMax, n).size())} {
    assert(_lMax >= std::abs(_n) && _lMax <= _grid.MaxDegree());
    assert(std::ranges::any_of(_grid.UpperIndices(),
                               [n](auto np) { return n == np; }));
  }

  CanonicalCoefficient(Int n, GSHGrid& grid)
      : CanonicalCoefficient(grid.MaxDegree(), n, grid) {}

  CanonicalCoefficient(CanonicalCoefficient&) = default;
  CanonicalCoefficient(CanonicalCoefficient&&) = default;

  template <typename Derived>
  CanonicalCoefficient(CanonicalCoefficientBase<Derived>& other)
      : _lMax{other.MaxDegree()},
        _n{other.UpperIndex()},
        _grid{other.grid()},
        _data{Vector(other.begin(), other.end())} {}

  template <typename Derived>
  CanonicalCoefficient(CanonicalCoefficientBase<Derived>&& other)
      : CanonicalCoefficient(other) {}

  template <typename Derived>
  requires std::same_as<typename Derived::coefficient_type,
                        coefficient_type> and
           std::same_as<typename Derived::value_type, value_type>
  auto& operator=(CanonicalCoefficientBase<Derived>& other) {
    assert(_lMax == other.MaxDegree());
    assert(std::abs(_n) == other.UpperIndex());
    _n = other.UpperIndex();
    std::ranges::copy(other.DataView(), _data.begin());
    return *this;
  }

  template <typename Derived>
  requires std::same_as<typename Derived::coefficient_type,
                        coefficient_type> and
           std::same_as<typename Derived::value_type, value_type>
  auto& operator=(CanonicalCoefficientBase<Derived>&& other) {
    *this = other;
    return *this;
  }

 private:
  Int _lMax;
  Int _n;
  GSHGrid& _grid;
  Vector _data;

  auto _DataView() { return boost::sub_range<Vector>(_data); }
  auto& _Grid() const { return _grid; }
  auto _MaxDegree() { return _lMax; }
  auto _UpperIndex() { return _n; }

  friend class CanonicalCoefficientBase<
      CanonicalCoefficient<GSHGrid, ValueType>>;
};

//-----------------------------------------------------------------//
//    Canonical coeffcient class that stores a view to its data    //
//-----------------------------------------------------------------//

template <typename GSHGrid, RealOrComplexValued ValueType,
          std::ranges::common_range View>
requires std::same_as<RemoveComplex<std::ranges::range_value_t<View>>,
                      typename GSHGrid::real_type>
class CanonicalCoefficientView
    : public CanonicalCoefficientBase<
          CanonicalCoefficientView<GSHGrid, ValueType, View>> {
  using Int = std::ptrdiff_t;

 public:
  using value_type = std::ranges::range_value_t<View>;
  using coefficient_type = ValueType;
  using MRange =
      std::conditional_t<std::same_as<ValueType, RealValued>, NonNegative, All>;

  CanonicalCoefficientView(Int lMax, Int n, GSHGrid& grid, View view, ValueType)
      : _lMax{lMax}, _n{n}, _grid{grid}, _view{view} {
    assert(_lMax >= std::abs(_n) && _lMax <= _grid.MaxDegree());
    assert(view.size() == GSHIndices<MRange>(_lMax, _lMax, n).size());
    assert(std::ranges::any_of(_grid.UpperIndices(),
                               [n](auto np) { return n == np; }));
  }

  CanonicalCoefficientView(Int lMax, Int n, GSHGrid& grid, View view)
      : CanonicalCoefficientView(lMax, n, grid, view, coefficient_type{}) {}

  template <typename Derived>
  requires std::same_as<typename Derived::coefficient_type,
                        coefficient_type> and
           std::same_as<typename Derived::value_type, value_type>
  auto& operator=(CanonicalCoefficientBase<Derived>& other) {
    assert(_lMax == other.MaxDegree());
    assert(std::abs(_n) == other.UpperIndex());
    _n = other.UpperIndex();
    std::ranges::copy(other.DataView(), _view.begin());
    return *this;
  }

 private:
  Int _lMax;
  Int _n;
  GSHGrid& _grid;
  View _view;

  auto _DataView() { return _view; }
  auto& _Grid() const { return _grid; }
  auto _MaxDegree() { return _lMax; }
  auto _UpperIndex() { return _n; }

  friend class CanonicalCoefficientBase<
      CanonicalCoefficientView<GSHGrid, ValueType, View>>;
};

//-------------------------------------------------------------//
//      Functions to transform canonical coefficient views     //
//-------------------------------------------------------------//

// Transform a view using unary operation.
template <typename Derived, typename Function>
auto CanonicalCoefficientUnaryOperation(CanonicalCoefficientBase<Derived>& view,
                                        Function f) {
  using coefficient_type = typename Derived::coefficient_type;
  return CanonicalCoefficientView(
      view.MaxDegree(), view.UpperIndex(), view.Grid(),
      view.DataView() | boost::adaptors::transformed(f), coefficient_type{});
}

template <typename Derived, typename Function>
auto CanonicalCoefficientUnaryOperation(
    CanonicalCoefficientBase<Derived>&& view, Function f) {
  return CanonicalCoefficientUnaryOperation(view, f);
}

// Overloads for unary minus.
template <typename Derived>
auto operator-(CanonicalCoefficientBase<Derived>& view) {
  return CanonicalCoefficientUnaryOperation(view, [](auto x) { return -x; });
}

template <typename Derived>
auto operator-(CanonicalCoefficientBase<Derived>&& view) {
  return -view;
}

// View formed from component-wise scalar operator
template <typename Derived, typename S, typename Function>
requires requires() {
  requires std::integral<S> or RealFloatingPoint<S> or
               (ComplexFloatingPoint<S> and
                std::same_as<typename Derived::coefficient_type,
                             ComplexValued>);
}
auto CanonicalCoefficientScalarOperation(
    CanonicalCoefficientBase<Derived>& view, S a, Function f) {
  using T = typename Derived::value_type;
  if constexpr (RealFloatingPoint<S> or std::integral<S>) {
    auto b = T(a);
    auto g = std::function<T(T)>([b, f](T x) { return f(x, b); });
    return CanonicalCoefficientUnaryOperation(view, g);
  } else {
    auto g = std::function<S(T)>([a, f](T x) { return f(S(x), a); });
    return CanonicalCoefficientUnaryOperation(view, g);
  }
}

template <typename Derived, typename S, typename Function>
auto CanonicalCoefficientScalarOperation(
    CanonicalCoefficientBase<Derived>&& view, S a, Function f) {
  return CanonicalCoefficientScalarOperation(view, a, f);
}

// Overloads for scalar multiplication.
template <typename Derived, typename S>
requires std::integral<S> or RealOrComplexFloatingPoint<S>
auto operator*(CanonicalCoefficientBase<Derived>& view, S a) {
  return CanonicalCoefficientScalarOperation(view, a, std::multiplies<>());
}

template <typename Derived, typename S>
requires std::integral<S> or RealOrComplexFloatingPoint<S>
auto operator*(CanonicalCoefficientBase<Derived>&& view, S a) {
  return view * a;
}

template <typename Derived, typename S>
requires std::integral<S> or RealOrComplexFloatingPoint<S>
auto operator*(S a, CanonicalCoefficientBase<Derived>& view) {
  return view * a;
}

template <typename Derived, typename S>
requires std::integral<S> or RealOrComplexFloatingPoint<S>
auto operator*(S a, CanonicalCoefficientBase<Derived>&& view) {
  return view * a;
}

// Overloads for scalar division.
template <typename Derived, typename S>
requires std::integral<S> or RealOrComplexFloatingPoint<S>
auto operator/(CanonicalCoefficientBase<Derived>& view, S a) {
  return CanonicalCoefficientScalarOperation(view, a, std::divides<>());
}

template <typename Derived, typename S>
auto operator/(CanonicalCoefficientBase<Derived>&& view, S a) {
  return view / a;
}

// Transform a pair of views using a binary operation.
template <typename Derived1, typename Derived2, typename Function>
requires std::same_as<typename Derived1::coefficient_type,
                      typename Derived2::coefficient_type>
auto CanonicalCoefficientBinaryOperation(
    CanonicalCoefficientBase<Derived1>& view1,
    CanonicalCoefficientBase<Derived2>& view2, Function f) {
  using coefficient_type = typename Derived1::coefficient_type;
  assert(view1.size() == view2.size());
  auto view = boost::combine(view1.DataView(), view2.DataView()) |
              boost::adaptors::transformed([f](auto pair) {
                auto x = boost::get<0>(pair);
                auto y = boost::get<1>(pair);
                return f(x, y);
              });
  return CanonicalCoefficientView(view1.MaxDegree(), view1.UpperIndex(),
                                  view1.Grid(), view, coefficient_type{});
}

// Overloads for addition.
template <typename Derived1, typename Derived2>
auto operator+(CanonicalCoefficientBase<Derived1>& view1,
               CanonicalCoefficientBase<Derived2>& view2) {
  return CanonicalCoefficientBinaryOperation(view1, view2, std::plus<>());
}

template <typename Derived1, typename Derived2>
auto operator+(CanonicalCoefficientBase<Derived1>&& view1,
               CanonicalCoefficientBase<Derived2>& view2) {
  return view1 + view2;
}

template <typename Derived1, typename Derived2>
auto operator+(CanonicalCoefficientBase<Derived1>& view1,
               CanonicalCoefficientBase<Derived2>&& view2) {
  return view1 + view2;
}

template <typename Derived1, typename Derived2>
auto operator+(CanonicalCoefficientBase<Derived1>&& view1,
               CanonicalCoefficientBase<Derived2>&& view2) {
  return view1 + view2;
}

// Overloads for subtraction.
template <typename Derived1, typename Derived2>
auto operator-(CanonicalCoefficientBase<Derived1>& view1,
               CanonicalCoefficientBase<Derived2>& view2) {
  return CanonicalCoefficientBinaryOperation(view1, view2, std::minus<>());
}

template <typename Derived1, typename Derived2>
auto operator-(CanonicalCoefficientBase<Derived1>&& view1,
               CanonicalCoefficientBase<Derived2>& view2) {
  return view1 - view2;
}

template <typename Derived1, typename Derived2>
auto operator-(CanonicalCoefficientBase<Derived1>& view1,
               CanonicalCoefficientBase<Derived2>&& view2) {
  return view1 - view2;
}

template <typename Derived1, typename Derived2>
auto operator-(CanonicalCoefficientBase<Derived1>&& view1,
               CanonicalCoefficientBase<Derived2>&& view2) {
  return view1 - view2;
}

*/

}  // namespace GSHTrans

#endif  // GSH_TRANS_CANONICAL_COEFFICIENTS_GUARD_H
