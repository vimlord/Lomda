#ifndef _LINALG_MATRIX_HPP
#define _LINALG_MATRIX_HPP

#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>

#include "stringable.hpp"

float abs(float);

class Matrix : public Stringable {
    protected:
        float *vals;
    public:
        int R, C;

        Matrix(int r, int c, float *vs = NULL);
        Matrix(const Matrix&);
        ~Matrix() { delete[] vals; }

        Matrix& operator=(Matrix& m);

        float& operator()(int r, int c) { return vals[r*C + c]; }

        Matrix operator+(Matrix& other);

        Matrix operator-();
        Matrix operator-(Matrix& other);

        Matrix operator*(Matrix& other);
        Matrix operator*(float other);

        Matrix operator/(Matrix&);
        Matrix operator/(float);

        bool operator==(Matrix& other);

        float determinant();
        Matrix gaussian();
        Matrix inverse();
        Matrix transpose();

        Matrix rowVector(int);
        Matrix colVector(int);

        std::string toString();
        
};

// Extra matrix operations
Matrix operator*(float, Matrix);
Matrix operator/(float, Matrix);

#endif
