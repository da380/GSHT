#ifndef GSH_TRANS_MATRIX_FIELD_GUARD_H
#define GSH_TRANS_MATRIX_FIELD_GUARD_H

#include <FFTWpp/Core>
#include <concepts>
#include <iostream>
#include <vector>

#include "Concepts.h"
#include "GridBase.h"
#include "Indexing.h"
#include "ScalarField.h"
#include "VectorField.h"

namespace GSHTrans {

//-------------------------------------------------//
//               Define the base class             //
//-------------------------------------------------//
template <typename _Derived>
class MatrixFieldBase : public FieldBase<MatrixFieldBase<_Derived>> {
  using Int = std::ptrdiff_t;

 public:
  // Methods related to the grid.
  auto GetGrid() const { return Derived().GetGrid(); }

  // Methods related to the data.
  auto size() const { return 9 * GetGrid().FieldSize(); }
  auto ComponentSize() const { return GetGrid().FieldSize(); }

  auto CanonicalIndices() const {
    return std::ranges::views::cartesian_product(
        std::ranges::views::iota(-1, 2), std::ranges::views::iota(-1, 2));
  }

  void CheckCanonicalIndices(Int alpha, Int beta) const {
    assert(std::abs(alpha) <= 1);
    assert(std::abs(beta) <= 1);
  }

  auto operator()(Int alpha, Int beta, Int iTheta, Int iPhi) const {
    return Derived().operator()(alpha, beta, iTheta, iPhi);
  }
  auto operator()(Int alpha, Int beta) const {
    return Derived().operator()(alpha, beta);
  }

  void Print() const {
    for (auto [iTheta, iPhi] : this->PointIndices()) {
      std::cout << iTheta << " " << iPhi << " ";
      for (auto [alpha, beta] : CanonicalIndices()) {
        std::cout << operator()(alpha, beta, iTheta, iPhi) << " ";
      }
      std::cout << std::endl;
    }
  }

 private:
  auto& Derived() const { return static_cast<const _Derived&>(*this); }
  auto& Derived() { return static_cast<_Derived&>(*this); }
};

//-------------------------------------------------//
//              Matrix component view              //
//-------------------------------------------------//
template <typename Derived>
class MatrixFieldComponentView
    : public ScalarFieldBase<MatrixFieldComponentView<Derived>> {
  using Int = std::ptrdiff_t;

 public:
  using Grid = typename Derived::Grid;
  using Scalar = typename Derived::Scalar;
  using Value = typename Derived::Value;
  using Real = typename Derived::Real;
  using Complex = typename Derived::Complex;

  // Methods needed to inherit from ScalarField Base.
  auto GetGrid() const { return _u.GetGrid(); }
  auto operator()(Int iTheta, Int iPhi) const {
    return _u(_alpha, _beta, iTheta, iPhi);
  }

  // Constructors.
  MatrixFieldComponentView(const MatrixFieldBase<Derived>& u, Int alpha,
                           Int beta)
      : _u{u}, _alpha{alpha}, _beta{beta} {}

  MatrixFieldComponentView(const MatrixFieldComponentView&) = default;
  MatrixFieldComponentView(MatrixFieldComponentView&&) = default;

  // Assignment.
  MatrixFieldComponentView& operator=(const MatrixFieldComponentView&) =
      default;
  MatrixFieldComponentView& operator=(MatrixFieldComponentView&&) = default;

 private:
  const VectorFieldBase<Derived>& _u;
  Int _alpha;
  Int _beta;
};

//-------------------------------------------------//
//       Matrix field that stores its data         //
//-------------------------------------------------//
template <typename _Grid, RealOrComplexValued _Value>
requires std::derived_from<_Grid, GridBase<_Grid>>
class MatrixField : public MatrixFieldBase<MatrixField<_Grid, _Value>> {
  using Int = std::ptrdiff_t;

 public:
  using Grid = _Grid;
  using Value = _Value;
  using Real = typename _Grid::Real;
  using Complex = typename _Grid::Complex;
  using Scalar =
      std::conditional_t<std::same_as<_Value, RealValued>, Real, Complex>;

  // Methods needed to inherit from MatrixField Base.
  auto GetGrid() const { return _grid; }
  auto operator()(Int alpha, Int beta, Int iTheta, Int iPhi) const {
    assert(std::abs(alpha) <= 1);
    assert(std::abs(beta) <= 1);
    return _data[Index(alpha, beta, iTheta, iPhi)];
  }
  auto operator()(Int alpha, Int beta) const {
    return MatrixFieldComponentView(*this, alpha, beta);
  }

  // Constructors.
  MatrixField() = default;

  MatrixField(_Grid grid)
      : _grid{grid}, _data{FFTWpp::vector<Scalar>(this->size())} {}

  MatrixField(_Grid grid, std::array<Scalar, 9>&& u) : MatrixField(grid) {
    for (auto [i, index] :
         std::ranges::views::enumerate(this->CanonicalIndices())) {
      auto [alpha, beta] = index;
      this->operator()(alpha, beta) = u[i];
    }
  }

  // Assignment.
  MatrixField& operator=(const MatrixField&) = default;
  MatrixField& operator=(MatrixField&&) = default;

  template <typename Derived>
  requires std::convertible_to<typename Derived::Scalar, Scalar> &&
           std::same_as<typename Derived::Value, Value>
  auto& operator=(const MatrixFieldBase<Derived>& other) {
    assert(this->size() == other.size());
    CopyValues(other);
    return *this;
  }

  template <typename Derived>
  requires std::convertible_to<typename Derived::Scalar, Scalar> &&
           std::same_as<typename Derived::Value, Value>
  auto& operator=(MatrixFieldBase<Derived>&& other) {
    *this = other;
    return *this;
  }

  // Iterators.
  auto begin() { return _data.begin(); }
  auto end() { return _data.end(); }

  // Value assignment.
  auto& operator()(Int alpha, Int beta, Int iTheta, Int iPhi) {
    return _data[Index(alpha, beta, iTheta, iPhi)];
  }
  auto operator()(Int alpha, Int beta) {
    auto start = std::next(begin(), Offset(alpha, beta));
    auto finish = std::next(start, this->ComponentSize());
    auto data = std::ranges::subrange(start, finish);
    return ScalarFieldView(_grid, data);
  }

 private:
  _Grid _grid;
  FFTWpp::vector<Scalar> _data;

  auto Offset(Int alpha, Int beta) const {
    return (3 * (alpha + 1) + (beta + 1)) * (this->ComponentSize());
  }

  template <typename Derived>
  void CopyValues(const MatrixFieldBase<Derived>& other) {
    for (auto [alpha, beta] : this->CanonicalIndices()) {
      for (auto [iTheta, iPhi] : this->PointIndices()) {
        operator()(alpha, beta, iTheta, iPhi) =
            other(alpha, beta, iTheta, iPhi);
      }
    }
  }

  auto Index(Int alpha, Int beta, Int iTheta, int iPhi) const {
    return Offset(alpha, beta) + iTheta * this->NumberOfLongitudes() + iPhi;
  }
};

//-------------------------------------------------//
//     Complexification of real matrix field       //
//-------------------------------------------------//
template <typename Derived>
requires std::same_as<typename Derived::Value, RealValued>
class ComplexifiedMatrixField
    : public MatrixFieldBase<ComplexifiedMatrixField<Derived>> {
  using Int = std::ptrdiff_t;

 public:
  using Grid = typename Derived::Grid;
  using Real = typename Grid::Real;
  using Complex = typename Grid::Complex;
  using Value = ComplexValued;
  using Scalar = Complex;

  // Methods needed to inherit from MatrixField Base.
  auto GetGrid() const { return _u.GetGrid(); }
  auto operator()(Int alpha, Int beta, Int iTheta, Int iPhi) const -> Complex {
    constexpr auto ii = Complex(0, 1);
    auto index = 3 * alpha + beta;
    if (index < 0) {
      return _u(-alpha, -beta, iTheta, iPhi) -
             ii * _u(alpha, beta, iTheta, iPhi);
    } else if (index == 0) {
      return _u(0, 0, iTheta, iPhi);
    } else {
      return _u(alpha, beta, iTheta, iPhi) +
             ii * _u(-alpha, -beta, iTheta, iPhi);
    }
  }

  auto operator()(Int alpha) const {
    return MatrixFieldComponentView(*this, alpha);
  }

  // Constructors.
  ComplexifiedMatrixField(const MatrixFieldBase<Derived>& u) : _u{u} {}

  ComplexifiedMatrixField(const ComplexifiedMatrixField&) = default;
  ComplexifiedMatrixField(ComplexifiedMatrixField&&) = default;

  // Assignment.
  ComplexifiedMatrixField& operator=(const ComplexifiedMatrixField&) = default;
  ComplexifiedMatrixField& operator=(ComplexifiedMatrixField&&) = default;

 private:
  const MatrixFieldBase<Derived>& _u;
};

//-------------------------------------------------//
//     Realification of complex matrix field       //
//-------------------------------------------------//
template <typename Derived>
requires std::same_as<typename Derived::Value, ComplexValued>
class RealifiedMatrixField
    : public MatrixFieldBase<RealifiedMatrixField<Derived>> {
  using Int = std::ptrdiff_t;

 public:
  using Grid = typename Derived::Grid;
  using Real = typename Grid::Real;
  using Complex = typename Grid::Complex;
  using Value = RealValued;
  using Scalar = Real;

  // Methods needed to inherit from MatrixField Base.
  auto GetGrid() const { return _u.GetGrid(); }
  auto operator()(Int alpha, Int beta, Int iTheta, Int iPhi) const {
    constexpr auto half = static_cast<Real>(1) / static_cast<Real>(2);
    auto index = 3 * alpha + beta;
    if (index < 0) {
      return half * std::imag(_u(-alpha, -beta, iTheta, iPhi) -
                              _u(alpha, beta, iTheta, iPhi));
    } else if (index == 0) {
      return std::real(_u(0, 0, iTheta, iPhi));
    } else {
      return half * std::real(_u(alpha, beta, iTheta, iPhi) +
                              _u(-alpha, -beta, iTheta, iPhi));
    }
  }

  auto operator()(Int alpha) const {
    return MatrixFieldComponentView(*this, alpha);
  }

  // Constructors.
  RealifiedMatrixField(const MatrixFieldBase<Derived>& u) : _u{u} {}

  RealifiedMatrixField(const RealifiedMatrixField&) = default;
  RealifiedMatrixField(RealifiedMatrixField&&) = default;

  // Assignment.
  RealifiedMatrixField& operator=(const RealifiedMatrixField&) = default;
  RealifiedMatrixField& operator=(RealifiedMatrixField&&) = default;

 private:
  const MatrixFieldBase<Derived>& _u;
};

//-------------------------------------------------//
//                 Unary expression                //
//-------------------------------------------------//
template <typename Derived, typename Function>
requires requires() {
  requires std::invocable<Function, typename Derived::Scalar>;
  requires std::convertible_to<
      std::invoke_result_t<Function, typename Derived::Scalar>,
      typename Derived::Scalar>;
}
class MatrixFieldUnary
    : public MatrixFieldBase<MatrixFieldUnary<Derived, Function>> {
  using Int = std::ptrdiff_t;

 public:
  using Grid = typename Derived::Grid;
  using Scalar = std::invoke_result_t<Function, typename Derived::Scalar>;
  using Value =
      std::conditional_t<RealFloatingPoint<Scalar>, RealValued, ComplexValued>;
  using Real = typename Derived::Real;
  using Complex = typename Derived::Complex;

  // Methods needed to inherit from MatrixField Base.
  auto GetGrid() const { return _u.GetGrid(); }
  auto operator()(Int alpha, Int beta, Int iTheta, Int iPhi) const {
    return _f(_u(alpha, beta, iTheta, iPhi));
  }
  auto operator()(Int alpha, Int beta) const {
    return MatrixFieldComponentView(*this, alpha, beta);
  }

  // Constructors.
  MatrixFieldUnary() = delete;
  MatrixFieldUnary(const MatrixFieldBase<Derived>& u, Function f)
      : _u{u}, _f{f} {}

  MatrixFieldUnary(const MatrixFieldUnary&) = default;
  MatrixFieldUnary(MatrixFieldUnary&&) = default;

  // Assignment.
  MatrixFieldUnary& operator=(MatrixFieldUnary&) = default;
  MatrixFieldUnary& operator=(MatrixFieldUnary&&) = default;

 private:
  const MatrixFieldBase<Derived>& _u;
  Function _f;
};

//-----------------------------------------------------//
//              MatrixField -> MatrixField             //
//-----------------------------------------------------//
template <typename Derived>
auto operator-(const MatrixFieldBase<Derived>& u) {
  return MatrixFieldUnary(u, [](auto x) { return -x; });
}

template <typename Derived>
auto operator-(MatrixFieldBase<Derived>&& u) {
  return -u;
}

//-----------------------------------------------------//
//         RealMatrixField -> ComplexMatrixField       //
//-----------------------------------------------------//
template <typename Derived>
requires std::same_as<typename Derived::Value, RealValued>
auto complex(const MatrixFieldBase<Derived>& u) {
  return ComplexifiedMatrixField(u);
}

template <typename Derived>
requires std::same_as<typename Derived::Value, RealValued>
auto complex(MatrixFieldBase<Derived>&& u) {
  return complex(u);
}

//-----------------------------------------------------//
//         ComplexMatrixField -> RealMatrixField       //
//-----------------------------------------------------//
template <typename Derived>
requires std::same_as<typename Derived::Value, ComplexValued>
auto real(const MatrixFieldBase<Derived>& u) {
  return RealifiedMatrixField(u);
}

template <typename Derived>
requires std::same_as<typename Derived::Value, ComplexValued>
auto real(MatrixFieldBase<Derived>&& u) {
  return real(u);
}

}  // namespace GSHTrans

#endif  // GSH_TRANS_MATRIX_FIELD_GUARD_H