
#include <GSHTrans/All>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <numbers>
#include <random>

int main() {
  using namespace GSHTrans;

  using Real = double;

  // Set the degree, order and upper index
  int lMax = 3;
  int mMax = lMax;
  int nMax = lMax;

  auto theta = double(1);

  auto d = Wigner<double, All, All, Ortho>(lMax, mMax, mMax, theta);

  auto p = d(0)(0);
}
