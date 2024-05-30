#ifndef GSH_TRANS_SCALAR_FIELD_POINTWISE_UNARY_WITH_SCALAR_GUARD_H
#define GSH_TRANS_SCALAR_FIELD_POINTWISE_UNARY_WITH_SCALAR_GUARD_H

#include <FFTWpp/Core>
#include <concepts>
#include <vector>

#include "../Concepts.h"
#include "../FieldBase.h"
#include "../GridBase.h"
#include "ScalarFieldBase.h"

namespace GSHTrans {

// Forward declare class.
template <typename Derived, typename Function>
requires requires() {
  requires std::invocable<Function, typename Derived::Scalar,
                          typename Derived::Scalar>;
  requires std::convertible_to<
      std::invoke_result_t<Function, typename Derived::Scalar,
                           typename Derived::Scalar>,
      typename Derived::Scalar>;
}
class ScalarFieldPointwiseUnaryWithScalar;

// Set traits.
namespace Internal {

template <typename Derived, typename Function>
struct Traits<ScalarFieldPointwiseUnaryWithScalar<Derived, Function>> {
  using Int = std::ptrdiff_t;
  using Grid = typename Derived::Grid;
  using Real = typename Grid::Real;
  using Complex = typename Grid::Complex;
  using Scalar = std::invoke_result_t<Function, typename Derived::Scalar,
                                      typename Derived::Scalar>;
  using Value =
      std::conditional_t<RealFloatingPoint<Scalar>, RealValued, ComplexValued>;
  using Writeable = std::false_type;
};

}  // namespace Internal

template <typename Derived, typename Function>
requires requires() {
  requires std::invocable<Function, typename Derived::Scalar,
                          typename Derived::Scalar>;
  requires std::convertible_to<
      std::invoke_result_t<Function, typename Derived::Scalar,
                           typename Derived::Scalar>,
      typename Derived::Scalar>;
}
class ScalarFieldPointwiseUnaryWithScalar
    : public ScalarFieldBase<
          ScalarFieldPointwiseUnaryWithScalar<Derived, Function>> {
 public:
  using Int = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Int;
  using Grid = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Grid;
  using Value = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Value;
  using Real = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Real;
  using Complex = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Complex;
  using Scalar = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Scalar;
  using Writeable = typename Internal::Traits<
      ScalarFieldPointwiseUnaryWithScalar<Derived, Function>>::Writeable;

  // Methods needed to inherit from ScalarField Base.
  auto GetGrid() const { return _u.GetGrid(); }
  auto operator()(Int iTheta, Int iPhi) const {
    this->CheckPointIndices(iTheta, iPhi);
    return _f(_u(iTheta, iPhi), _s);
  }

  // Constructors.
  ScalarFieldPointwiseUnaryWithScalar() = delete;
  ScalarFieldPointwiseUnaryWithScalar(const ScalarFieldBase<Derived>& u,
                                      Function f, Scalar s)
      : _u{u}, _f{f}, _s{s} {}

  ScalarFieldPointwiseUnaryWithScalar(
      const ScalarFieldPointwiseUnaryWithScalar&) = default;
  ScalarFieldPointwiseUnaryWithScalar(ScalarFieldPointwiseUnaryWithScalar&&) =
      default;

  // Assignment.
  ScalarFieldPointwiseUnaryWithScalar& operator=(
      ScalarFieldPointwiseUnaryWithScalar&) = default;
  ScalarFieldPointwiseUnaryWithScalar& operator=(
      ScalarFieldPointwiseUnaryWithScalar&&) = default;

 private:
  const ScalarFieldBase<Derived>& _u;
  Function _f;
  Scalar _s;
};

}  // namespace GSHTrans

#endif  // GSH_TRANS_SCALAR_FIELD_POINTWISE_UNARY_WITH_SCALAR_EXPRESSIONS_GUARD_H