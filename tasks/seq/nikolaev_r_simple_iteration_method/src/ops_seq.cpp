#include "seq/nikolaev_r_simple_iteration_method/include/ops_seq.hpp"

#include <iostream>

bool nikolaev_r_simple_iteration_method_seq::SimpleIterationMethodSequential::pre_processing() {
  internal_order_test();
  size_t n = *reinterpret_cast<size_t*>(taskData->inputs[0]);
  A_.assign(n * n, 0.0);
  b_.assign(n, 0.0);
  x_.assign(n, 0.0);
  auto* A_data = reinterpret_cast<double*>(taskData->inputs[1]);
  auto* b_data = reinterpret_cast<double*>(taskData->inputs[2]);
  std::copy(A_data, A_data + n * n, A_.begin());
  std::copy(b_data, b_data + n, b_.begin());
  return true;
}

bool nikolaev_r_simple_iteration_method_seq::SimpleIterationMethodSequential::validation() {
  internal_order_test();
  if (taskData->inputs_count.size() != 3 || taskData->outputs_count.size() != 1) {
    std::cerr << "Error: Incorrect amount of inputs and outputs." << std::endl;
    return false;
  }
  size_t n = *reinterpret_cast<size_t*>(taskData->inputs[0]);
  if (n < 1) {
    std::cerr << "Error: Incorrect matrix size." << std::endl;
    return false;
  }
  auto* A_data = reinterpret_cast<double*>(taskData->inputs[1]);
  std::vector<double> A(A_data, A_data + n * n);
  if (is_singular(A, n)) {
    std::cerr << "Error: Matrix is singular." << std::endl;
    return false;
  }
  if (!is_diagonally_dominant(A, n)) {
    std::cerr << "Error: Matrix is not diagonally dominant." << std::endl;
    return false;
  }
  return true;
}

bool nikolaev_r_simple_iteration_method_seq::SimpleIterationMethodSequential::run() {
  internal_order_test();
  size_t n = b_.size();

  std::vector<double> B(n * n, 0.0);
  std::vector<double> g(n, 0.0);

  for (size_t i = 0; i < n; ++i) {
    if (A_[i * n + i] == 0) {
      std::cerr << "Error: Zero diagonal element detected in matrix A." << std::endl;
      return false;
    }
    for (size_t j = 0; j < n; ++j) {
      if (i == j) {
        B[i * n + j] = 0;
      } else {
        B[i * n + j] = -A_[i * n + j] / A_[i * n + i];
      }
    }
    g[i] = b_[i] / A_[i * n + i];
  }
  std::vector<double> x_old(n, 0.0);
  for (size_t iter = 0; iter < max_iterations_; ++iter) {
    std::vector<double> x_new(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
      x_new[i] = g[i];
      for (size_t j = 0; j < n; ++j) {
        x_new[i] += B[i * n + j] * x_old[j];
      }
    }
    double max_diff = 0.0;
    for (size_t i = 0; i < n; ++i) {
      max_diff = std::max(max_diff, fabs(x_new[i] - x_old[i]));
    }
    if (max_diff < tolerance_) {
      x_ = x_new;
      return true;
    }
    x_old = x_new;
  }
  std::cerr << "Error: Method did not converge within the maximum number of iterations." << std::endl;
  return false;
}

bool nikolaev_r_simple_iteration_method_seq::SimpleIterationMethodSequential::post_processing() {
  internal_order_test();
  for (size_t i = 0; i < x_.size(); i++) {
    reinterpret_cast<double*>(taskData->outputs[0])[i] = x_[i];
  }
  return true;
}

bool nikolaev_r_simple_iteration_method_seq::is_singular(const std::vector<double>& A, size_t n) {
  std::vector<std::vector<double>> mat(n, std::vector<double>(n));
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < n; ++j) {
      mat[i][j] = A[i * n + j];
    }
  }
  double det = 1;
  for (size_t i = 0; i < n; ++i) {
    if (mat[i][i] == 0) {
      bool found = false;
      for (size_t k = i + 1; k < n; ++k) {
        if (mat[k][i] != 0) {
          swap(mat[i], mat[k]);
          det *= -1;
          found = true;
          break;
        }
      }
      if (!found) {
        return true;
      }
    }
    for (size_t j = i + 1; j < n; ++j) {
      double ratio = mat[j][i] / mat[i][i];
      for (size_t k = i; k < n; ++k) {
        mat[j][k] -= ratio * mat[i][k];
      }
    }
    det *= mat[i][i];
  }
  return det == 0;
}

bool nikolaev_r_simple_iteration_method_seq::is_diagonally_dominant(const std::vector<double>& A, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    double diagonal_element = fabs(A[i * n + i]);
    double sum = 0.0;

    for (size_t j = 0; j < n; ++j) {
      if (j != i) {
        sum += fabs(A[i * n + j]);
      }
    }

    if (diagonal_element <= sum) {
      return false;
    }
  }
  return true;
}