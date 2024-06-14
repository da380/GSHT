#ifndef GSH_TRANS_ABSTRACT_VECTOR_FIELD_GUARD_H
#define GSH_TRANS_ABSTARCT_VECTOR_FIELD_GUARD_H

#include <FFTWpp/Core>
#include <concepts>
#include <iostream>
#include <limits>
#include <vector>

#include "../Concepts.h"
#include "../FieldBase.h"
#include "../GridBase.h"
#include "../ScalarField/ScalarFieldBase.h"
#include "VectorFieldBase.h"
#include "VectorFieldComponent.h"
#include "VectorFieldConstComponent.h"

namespace GSHTrans {

// Forward declare class.
template <typename _Grid, RealOrComplexValued _Value, typename _Function>
requires std::derived_from<_Grid, GridBase<_Grid>> &&
         CanonicalVectorValuedFunction<_Function, typename _Grid::Real, _Value>
class AbstractVectorField;

// Set traits.
namespace Internal {

template <typename _Grid, RealOrComplexValued _Value, typename _Function>
struct Traits<AbstractVectorField<_Grid, _Value, _Function>> {
  using Int = std::ptrdiff_t;
  using Grid = _Grid;
  using Value = _Value;
  using Real = typename Grid::Real;
  using Complex = typename Grid::Complex;
  using Scalar =
      std::conditional_t<std::same_as<Value, RealValued>, Real, Complex>;
  using Writeable = std::false_type;
};

}  // namespace Internal
template <typename _Grid, RealOrComplexValued _Value, typename _Function>
requires std::derived_from<_Grid, GridBase<_Grid>> &&
         CanonicalVectorValuedFunction<_Function, typename _Grid::Real, _Value>
class AbstractVectorField
    : public VectorFieldBase<AbstractVectorField<_Grid, _Value, _Function>> {
 public:
  using Grid = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Grid;
  using Value = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Value;
  using Int = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Int;
  using Real = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Real;
  using Complex = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Complex;
  using Scalar = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Scalar;
  using Writeable = typename Internal::Traits<
      AbstractVectorField<_Grid, _Value, _Function>>::Writeable;

  // Return the grid.
  auto GetGrid() const { return _grid; }

  // Read access to data.
  auto operator[](Int alpha, Int iTheta, Int iPhi) const {
    this->CheckCanonicalIndices(alpha);
    this->CheckPointIndices(iTheta, iPhi);
    auto theta = this->CoLatitudes()[iTheta];
    auto phi = this->Longitudes()[iPhi];
    return _f(theta, phi)[alpha];
  }

  // Return read access component
  auto operator[](Int alpha) const {
    this->CheckCanonicalIndices(alpha);
    return VectorFieldConstComponent(*this, alpha);
  }

  // Default constructor.
  AbstractVectorField() = default;

  // Construct from grid initialising values to zero.
  AbstractVectorField(_Grid grid, _Function&& f) : _grid{grid}, _f{f} {}

  // Default copy and move constructors.
  AbstractVectorField(const AbstractVectorField&) = default;
  AbstractVectorField(AbstractVectorField&&) = default;

  // Default copy and move assigment.
  AbstractVectorField& operator=(const AbstractVectorField&) = default;
  AbstractVectorField& operator=(AbstractVectorField&&) = default;

 private:
  _Grid _grid;
  _Function& _f;
};

// Type aliases for real and complex fields.
template <typename Grid, typename Function>
using RealAbstractVectorField = AbstractVectorField<Grid, RealValued, Function>;

template <typename Grid, typename Function>
using ComplexAbstractVectorField =
    AbstractVectorField<Grid, ComplexValued, Function>;

}  // namespace GSHTrans

#endif