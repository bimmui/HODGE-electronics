/*
  algebra.h: This file contains a number of utilities useful for handling
  3D vectors

  This work is an adaptation from vvector.h, written by Linas Vepstras. The
  original code can be found at:

  https://github.com/markkilgard/glut/blob/master/lib/gle/vvector.h

  HISTORY:
  Written by Linas Vepstas, August 1991
  Added 2D code, March 1993
  Added Outer products, C++ proofed, Linas Vepstas October 1993
  Adapted for altitude estimation tasks by Juan Gallostra June 2018
  Separated .h, .cpp by Simon D. Levy July 2018
*/

#pragma once

// #include <cmath>
#include <math.h>

// Copy 3D vector
void copy_vector(float b[3], float a[3]);

// Vector difference
void subtract_vectors(float v21[3], float v2[3], float v1[3]);

// Vector sum
void sum_vectors(float v21[3], float v2[3], float v1[3]);

// scalar times vector
void scale_vector(float c[3], float a, float b[3]);

// accumulate scaled vector
void accumulate_scaledvector(float c[3], float a, float b[3]);

// Vector dot product
void dotproduct_vectors(float *c, float a[3], float b[3]);

// Vector length
void vector_length(float *len, float a[3]);

// Normalize vector
void normalize_vector(float a[3]);

// 3D Vector cross product yeilding vector
void crossproduct_vectors(float c[3], float a[3], float b[3]);

// initialize matrix
void identity_matrix3x3(float m[3][3]);

// matrix copy
void copy_matrix3x3(float b[3][3], float a[3][3]);

// matrix transpose
void transpose_matrix3x3(float b[3][3], float a[3][3]);

// multiply matrix by scalar
void scale_matrix3x3(float b[3][3], float s, float a[3][3]);

// multiply matrix by scalar and add result to another matrix
void scaleaccumulate_matrix3x3(float b[3][3], float s, float a[3][3]);

// matrix product
// c[x][y] = a[x][0]*b[0][y]+a[x][1]*b[1][y]+a[x][2]*b[2][y]+a[x][3]*b[3][y]
void matrix_product3x3(float c[3][3], float a[3][3], float b[3][3]);

// matrix times vector
void matrix_dotvector3x3(float p[3], float m[3][3], float v[3]);

// determinant of matrix
// Computes determinant of matrix m, returning d
void determinant3x3(float *d, float m[3][3]);

// adjoint of matrix
// Computes adjoint of matrix m, returning a
// (Note that adjoint is just the transpose of the cofactor matrix);
void adjoint3x3(float a[3][3], float m[3][3]);

// compute adjoint of matrix and scale
// Computes adjoint of matrix m, scales it by s, returning a
void scale_adjoint3x3(float a[3][3], float s, float m[3][3]);

// inverse of matrix
// Compute inverse of matrix a, returning determinant m and
// inverse b
bool invert3x3(float b[3][3], float a[3][3]);

// skew matrix from vector
void skew(float a[3][3], float v[3]);

void print_matrix3X3(float mmm[3][3]);

void vec_print(float a[3]);
