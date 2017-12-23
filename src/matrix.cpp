#include "linalg/matrix.hpp"

#include <string>
#include <iostream>

#include <ostream>

float abs(float x) {
    return x > 0 ? x : -x;
}

Matrix::Matrix(int r, int c, float *vs) : R(r), C(c) {
    vals = new float[R*C];

    if (vs)
        for (int i = R*C-1; i >= 0; i--)
            vals[i] = vs[i];
}

Matrix::Matrix(const Matrix& m) {
    R = m.R;
    C = m.C;
    vals = new float[R*C];
    for (int i = R*C-1; i >= 0; i--)
        vals[i] = m.vals[i];
}

Matrix& Matrix::operator=(Matrix& m) {
    delete[] vals;

    R = m.R;
    C = m.C;
    vals = new float[R*C];
    for (int i = R*C-1; i >= 0; i--)
        vals[i] = m.vals[i];
    return *this;
}

Matrix Matrix::operator+(Matrix& other) {
    assert(R == other.R && C == other.C);
    Matrix res(R, C);
    for (int i = R*C; i >= 0; i--)
        res.vals[i] = vals[i] + other.vals[i];
    return res;
}

Matrix Matrix::operator-() {
    Matrix res(R, C);
    for (int i = R*C; i >= 0; i--)
        res.vals[i] = -vals[i];
    return res;
}

Matrix Matrix::operator-(Matrix& other) {
    assert(R == other.R && C == other.C);
    Matrix res(R, C);
    for (int i = R*C; i >= 0; i--)
        res.vals[i] = vals[i] - other.vals[i];
    return res;
}

Matrix Matrix::operator*(Matrix& other) {
    assert(C == other.R);
    Matrix M(R, other.C);

    for (int i = 0; i < R; i++)
        for (int j = 0; j < other.C; j++) {
            M(i, j) = 0;
            for (int k = 0; k < C; k++)
                M(i, j) += (*this)(i, k) * other(k, j);
        }

    return M;
}

bool Matrix::operator==(Matrix &other) {
    if (R != other.R || C != other.C) return false;
    else for (int i = R*C-1; i >= 0; i--)
        if (vals[i] != other.vals[i])
            return false;

    return true;
}

float Matrix::determinant() {
    assert(R == C);

    if (R == 2)
        return vals[0] * vals[3] - vals[1] * vals[2];
    else {
        float det = 0;
        Matrix M(R-1, R-1);
        for (int i = 0; i < R-1; i++)
            for (int j = 0; j < R-1; j++)
                M(i, j) = (*this)(i+1, j+1);
        
        int sign = 1;
        for (int i = 0; i < R; i++) {
            det += (*this)(0, i) * M.determinant() * sign;
            sign *= 1;
            if (i + 1 == R) for (int j = 0; j < R-1; j++)
                M(i, j) = (*this)(i, j+1);
        }

        return det;

    }

}

Matrix Matrix::gaussian() {
    Matrix M(R,C,vals);

    int rank = R > C ? C : R;

    // Ensure that the matrix is upper triangular
    for (int k = 0, p = 0; p < rank; k++, p++) {
        // Find the row with the largest pivot
        int h = p;
        for (int i = p+1; i < rank; i++)
            if (abs(M(h, k)) < abs(M(i, k)))
                h = i;
        
        if (M(h, k) == 0) {
            // Evidence to singularity
            p--;
            rank--;
        } else {
            // Then, swap it into place
            if (M(p, k) == 0) {
                for (int i = k; i < C; i++) {
                    float t = M(p, i);
                    M(p, i) = M(h, i);
                    M(h, i) = t;
                }
            }
            
            // Now, we perform row additions
            for (int i = p+1; i < R; i++) {
                float f = M(i, k) / M(p, k);
                for (int j = k+1; j < C; j++)
                    M(i, j) -= M(p, j) * f;
                M(i, k) = 0;
            }
            
            // Finally, we seek to ensure that our coefficients are all one
            for (int i = C-1; i >= k; i--)
                M(p, i) /= M(p, k);
                

        }
    }

    // Reduce to diagonality
    for (int k = rank - 1; k >= 0; k--) {
        for (int i = 0; i < k; i++) {
            if (M(i, k) != 0) {
                float f = M(i, k) / M(k, k);
                for (int j = rank-1; j < C; j++)
                    M(i, j) -= M(k, j) * f;

            }
        }
    }

    return M;
}

Matrix Matrix::inverse() {
    assert(R == C);
    Matrix AI(R, R);
    for (int i = 0; i < R; i++)
        for (int j = 0; j < C; j++) {
            AI(i, j) = (*this)(i, j);
            AI(i, C+j) = i == j ? 1 : 0;
        }
    
    // Perform gaussian reduction on augmentation AI
    Matrix IB = AI.gaussian();

    // Verify the invertability of the matrix
    for (int i = 0; i < R; i++)
        if (IB(i, i) != 1) {
            // The result is noncomputable
            Matrix B(R, R);
            for (int j = R*R-1; j >= 0; j--)
                B.vals[j] = NAN;
            return B;
        }

    Matrix B(R, R);
    for (int i = 0; i < C; i++)
        for (int j = 0; j < R; j++)
            B(i, j) = IB(i, j+C);
    
    return B;
}

std::string Matrix::toString() {
    std::string s = "";

    for (int r = 0; r < R; r++) {
        if (r)
            s += "\n";

        s += "|";
        for (int c = 0; c < C; c++) {
            if (c) s += ", ";
            s += std::to_string((*this)(r, c));
        }
        s += "|";
    }

    return s;
}    

Matrix Matrix::transpose() {
    Matrix T(C, R);
    for (int r = 0; r < R; r++)
        for (int c = 0; c < C; c++)
            T(c, r) = (*this)(r, c);
    return T;
}

Matrix Matrix::colVector(int c) {
    Matrix col(R,1);
    for (int i = 0; i < R; i++)
        col.vals[i] = (*this)(i, c);
    return col;
}

Matrix Matrix::rowVector(int r) {
    Matrix row(1,C);
    for (int i = 0; i < C; i++)
        row.vals[i] = (*this)(r, i);
    return row;
}


