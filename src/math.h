#pragma once
#include "types.h"
#include <cmath>
#include <initializer_list>

static constexpr f32 PI = 3.14159265358979323846f;

template<typename T, size_t N>
class Vector {
  public:
    constexpr Vector(std::initializer_list<T> values) {
        size_t i = 0;
        for(const auto& value : values) {
            data[i++] = value;
            if(i >= N) {
                break;
            }
        }
    }

    T operator[](size_t index) const {
        return data[index];
    }

    T& operator[](size_t index) {
        return data[index];
    }

    Vector<T, N> operator+(T scalar) const {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] + scalar;
        }
        return result;
    }

    Vector<T, N> operator+(const Vector<T, N>& other) const {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }

    Vector<T, N> operator-(T scalar) const {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] - scalar;
        }
        return result;
    }

    Vector<T, N> operator-(const Vector<T, N>& other) const {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] - other[i];
        }
        return result;
    }

    Vector<T, N> operator*(T scalar) {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] * scalar;
        }
        return result;
    }

    Vector<T, N> operator*(Vector<T, N> other) {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] * other[i];
        }
        return result;
    }

    T magnitude() {
        T magnitude = 0;
        for(size_t i = 0; i < N; ++i) {
            magnitude += data[i] * data[i];
        }
        return sqrtf(magnitude);
    }

    Vector<T, N> normalize() {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = data[i] / magnitude();
        }
        return result;
    }

    T dot(const Vector<T, N>& other) const {
        T sum = 0;
        for(size_t i = 0; i < N; ++i) {
            sum += data[i] * other[i];
        }
        return sum;
    }

    T negate() {
        Vector<T, N> result = { 0 };
        for(size_t i = 0; i < N; ++i) {
            result[i] = -data[i];
        }
        return result;
    }

  private:
    T data[N];
};

using Vec2 = Vector<f32, 2>;
using Vec3 = Vector<f32, 3>;
using Vec4 = Vector<f32, 4>;

template<typename T, size_t C, size_t R>
struct Matrix {
    T data[C][R];

    auto operator[](size_t index) -> T (&)[R] {
        return data[index];
    }

    Matrix<T, C, R> operator*(const T scalar) {
        for(size_t i = 0; i < C; ++i) {
            for(size_t j = 0; j < R; ++j) {
                data[i][j] *= scalar;
            }
        }
    }

    Matrix<T, C, R> operator*(const Matrix<T, C, R>& other) {
        Matrix<T, C, R> result = {};
        for(size_t i = 0; i < C; ++i) {
            for(size_t j = 0; j < R; ++j) {
                T sum = 0;
                for(size_t k = 0; k < R; ++k) {
                    sum += data[i][k] * other.data[k][j];
                }
                result.data[i][j] = sum;
            }
        }
        return result;
    }
};

using Mat2 = Matrix<f32, 2, 2>;
using Mat3 = Matrix<f32, 3, 3>;
using Mat4 = Matrix<f32, 4, 4>;

// clang-format off
static constexpr Mat3 MAT3_IDENTITY = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

static constexpr Mat4 MAT4_IDENTITY = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};
// clang-format on