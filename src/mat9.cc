/*
 * Copyright (c) 2021 Goffredo Baroncelli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdio>
#include <cstring>

#include "mat9.hpp"

void mat9_invert(const Mat9 &m, Mat9 &minv) {
    /*
     * from https://stackoverflow.com/questions/983999/simple-3x3-matrix-inverse-code-c
     * with some simplification
     */
    const float m4857 = m[4] * m[8] - m[5] * m[7];
    const float m3746 = m[3] * m[7] - m[4] * m[6];
    const float m5638 = m[5] * m[6] - m[3] * m[8];
    const float det = m[0] * (m4857) +
                 m[1] * (m5638) +
                 m[2] * (m3746);

    const float invdet = 1 / det;

    //Matrix33d minv; // inverse of matrix m
    minv[0] = (m4857) * invdet;
    minv[1] = (m[2] * m[7] - m[1] * m[8]) * invdet;
    minv[2] = (m[1] * m[5] - m[2] * m[4]) * invdet;
    minv[3] = (m5638) * invdet;
    minv[4] = (m[0] * m[8] - m[2] * m[6]) * invdet;
    minv[5] = (m[2] * m[3] - m[0] * m[5]) * invdet;
    minv[6] = (m3746) * invdet;
    minv[7] = (m[1] * m[6] - m[0] * m[7]) * invdet;
    minv[8] = (m[0] * m[4] - m[1] * m[3]) * invdet;
}

void mat9_product(const Mat9 &m1, const Mat9 &m2, Mat9 &m3){
    int i,j, k;
    for (i = 0 ; i < 3 ; i++) {
        for (j = 0 ; j < 3 ; j++) {
            float sum = 0;
            for (k = 0 ; k < 3 ; k++)
                sum += m1[i*3+k]*m2[j+k*3];
            m3[i*3+j] = sum;
        }
    }
}

void mat9_sum(const Mat9 &m1, Mat9 &m2){
    int i;
    for (i = 0 ; i < 9 ; i++)
        m2[i] += m1[i];
}

void mat9_product(const float c, Mat9 &m1){
    int i;
    for (i = 0 ; i < 9 ; i++)
        m1[i] *= c;
}

void mat9_print(const Mat9 &m) {
    int i,j;
    for (i = 0 ; i < 3 ; i++ ) {
        printf("\t[");
        for (j = 0 ; j < 3 ; j++) {
            if (j != 0)
                printf(", ");
            printf("%f", m[i*3+j]);
        }
        printf("]\n");
    }
}

void mat9_set_identity(Mat9 &m) {
    static const float id[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
    memcpy(m.coeff, id, sizeof(m.coeff));
}

void mat9_set_translate(Mat9 &m, float dx, float dy) {
    float translate[] = {
        1, 0, dx,
        0, 1, dy,
        0, 0, 1
    };
    memcpy(m.coeff, translate, sizeof(m.coeff));
};

void mat9_set_scale(Mat9 &m, float sx, float sy) {
    float scale[] = {
        sx, 0,  0,
        0,  sy, 0,
        0,  0,  1
    };
    memcpy(m.coeff, scale, sizeof(m.coeff));
};


Mat9 Mat9::operator *(const Mat9 &other) const {
    Mat9 res;
    mat9_product(*this, other, res);
    return res;
}
Mat9 & Mat9::operator *=(const Mat9 &other) {
    Mat9 res = *this;
    mat9_product(res, other, *this);
    return *this;
}
Mat9 Mat9::operator +(const Mat9 &other) const {
    Mat9 res = *this;
    mat9_sum(other, res);
    return res;
}
Mat9 & Mat9::operator +=(const Mat9 &other) {
    mat9_sum(other, *this);
    return *this;
}

Mat9 Mat9::operator *(float other) const {
    Mat9 res = *this;
    mat9_product(other, res);
    return res;
}
Mat9 & Mat9::operator *=(float other) {
    mat9_product(other, *this);
    return *this;
}
Mat9 Mat9::invert() const {
    Mat9 res;
    mat9_invert(*this, res);
    return res;
}

Mat9 Mat9::identity_matrix() {
    Mat9 res;
    mat9_set_identity(res);
    return res;
}
Mat9 Mat9::translate_matrix(float dx, float dy) {
    Mat9 res;
    mat9_set_translate(res, dx, dy);
    return res;
}
Mat9 Mat9::scale_matrix(float sx, float sy) {
    Mat9 res;
    mat9_set_scale(res, sx, sy);
    return res;
}

Mat9 operator *(float lth, const Mat9 &rhs) {
    return rhs * lth;
}

#ifdef TEST_MAT9

#include <cassert>
#include <cstdio>

void test_mat9_set_identity() {
    Mat9 mat;
    mat9_set_identity(mat);
    assert(mat.coeff[0] == 1 && mat.coeff[4] == 1 && mat.coeff[8] == 1);
    assert(mat.coeff[1] == 0 && mat.coeff[2] == 0 && mat.coeff[3] == 0 &&
           mat.coeff[5] == 0 && mat.coeff[6] == 0 && mat.coeff[7] == 0);
}

void test_mat9_set_translate() {
    Mat9 mat;
    mat9_set_translate(mat, 4, 5);
    assert(mat.coeff[0] == 1 && mat.coeff[4] == 1 && mat.coeff[8] == 1);
    assert(mat.coeff[1] == 0 && mat.coeff[2] == 4 && mat.coeff[3] == 0 &&
           mat.coeff[5] == 5 && mat.coeff[6] == 0 && mat.coeff[7] == 0);
}

void test_mat9_set_scale() {
    Mat9 mat;
    mat9_set_scale(mat, 4, 5);
    assert(mat.coeff[0] == 4 && mat.coeff[4] == 5 && mat.coeff[8] == 1);
    assert(mat.coeff[1] == 0 && mat.coeff[2] == 0 && mat.coeff[3] == 0 &&
           mat.coeff[5] == 0 && mat.coeff[6] == 0 && mat.coeff[7] == 0);
}

void test_mat9_sum() {
    Mat9 mat1, mat2;
    mat9_set_scale(mat1, 4, 5);
    mat9_set_translate(mat2, 7, 8);
    mat9_sum(mat1, mat2);

    assert(mat1.coeff[0] == 4 && mat1.coeff[4] == 5 && mat1.coeff[8] == 1);
    assert(mat1.coeff[1] == 0 && mat1.coeff[2] == 0 && mat1.coeff[3] == 0 &&
           mat1.coeff[5] == 0 && mat1.coeff[6] == 0 && mat1.coeff[7] == 0);

    assert(mat2.coeff[0] == 5 && mat2.coeff[4] == 6 && mat2.coeff[8] == 2);
    assert(mat2.coeff[1] == 0 && mat2.coeff[2] == 7 && mat2.coeff[3] == 0 &&
           mat2.coeff[5] == 8 && mat2.coeff[6] == 0 && mat2.coeff[7] == 0);

}

void test_mat9_product_scalar() {
    Mat9 mat;
    mat9_set_scale(mat, 4, 5);
    mat9_product(4, mat);

    assert(mat.coeff[0] == 4*4 && mat.coeff[4] == 5*4 && mat.coeff[8] == 1*4);
    assert(mat.coeff[1] == 0 && mat.coeff[2] == 0 && mat.coeff[3] == 0 &&
           mat.coeff[5] == 0 && mat.coeff[6] == 0 && mat.coeff[7] == 0);

}

void test_mat9_product() {
    Mat9 mat1, mat2, out;
    mat9_set_translate(mat1, 4, 5);
    mat9_set_scale(mat2, 7, 8);
    mat9_product(mat1, mat2, out);

    assert(mat1.coeff[0] == 1 && mat1.coeff[4] == 1 && mat1.coeff[8] == 1);
    assert(mat1.coeff[1] == 0 && mat1.coeff[2] == 4 && mat1.coeff[3] == 0 &&
           mat1.coeff[5] == 5 && mat1.coeff[6] == 0 && mat1.coeff[7] == 0);

    assert(mat2.coeff[0] == 7 && mat2.coeff[4] == 8 && mat2.coeff[8] == 1);
    assert(mat2.coeff[1] == 0 && mat2.coeff[2] == 0 && mat2.coeff[3] == 0 &&
           mat2.coeff[5] == 0 && mat2.coeff[6] == 0 && mat2.coeff[7] == 0);

    assert(out.coeff[0] == 7 && out.coeff[1] == 0 && out.coeff[2] == 4 &&
           out.coeff[3] == 0 && out.coeff[4] == 8 && out.coeff[5] == 5 &&
           out.coeff[6] == 0 && out.coeff[7] == 0 && out.coeff[8] == 1);
}

void test_mat9_invert() {
    Mat9 mat1, mat2, out, mat2_inv, res;

    mat9_set_translate(mat1, 4, 5);
    mat9_set_scale(mat2, 7, 8);
    mat9_product(mat1, mat2, out);

    mat9_invert(mat2, mat2_inv);
    mat9_product(out, mat2_inv, res);

    for (int i = 0 ; i < 9 ; i++) {
        assert(mat1.coeff[i] == res.coeff[i]);
    }

}

void test_Mat9_access() {
    Mat9 mat1;

    mat1.coeff[0] = 4;
    mat1.coeff[1] = 5;

    const Mat9 mat2 = mat1;

    assert(mat2[0] == 4);
    assert(mat2[1] == 5);

    mat1[1] = 7;
    assert(mat1[1] == 7);

}

void test_Mat9_set() {
    Mat9 mat1;

    mat1.set(4, 5, 6, 7, 8, 10, 11, 13, 17);

    assert(mat1[0] == 4);
    assert(mat1[1] == 5);
    assert(mat1[8] == 17);
}

void test_Mat9_ctor() {
    Mat9 mat1;

    mat1.set(4, 5, 6, 7, 8, 10, 11, 13, 27);

    assert(mat1[0] == 4);
    assert(mat1[1] == 5);
    assert(mat1[8] == 27);
}

void test_Mat9_eq() {
    Mat9 mat1(4, 5, 6, 7, 8, 10, 11, 13, 19);
    Mat9 mat2(4, 5, 6, 7, 8, 10, 11, 13, 18);

    assert(!(mat1 == mat2));
    assert(mat1 != mat2);
    mat2[8] = 19;
    assert(mat1 == mat2);
}

void test_Mat9_set_identity() {
    Mat9 mat1;
    Mat9 mat2;

    mat9_set_identity(mat1);
    mat2.set_identity();

    assert(mat1 == mat2);
}

void test_Mat9_set_scale() {
    Mat9 mat1;
    Mat9 mat2;

    mat9_set_scale(mat1, 4, 7);
    mat2.set_scale(4, 7);

    assert(mat1 == mat2);
}

void test_Mat9_set_translate() {
    Mat9 mat1;
    Mat9 mat2;

    mat9_set_translate(mat1, 3, 7);
    mat2.set_translate(3, 7);

    assert(mat1 == mat2);
}

void test_Mat9_identity_matrix() {
    Mat9 mat1;
    mat9_set_identity(mat1);

    assert(mat1 == Mat9::identity_matrix());
}

void test_Mat9_scale_matrix() {
    Mat9 mat1;

    mat9_set_scale(mat1, 4, 7);
    assert(mat1 == Mat9::scale_matrix(4, 7));
}

void test_Mat9_translate_matrix() {
    Mat9 mat1;

    mat9_set_translate(mat1, 3, 7);
    assert(mat1 == Mat9::translate_matrix(3, 7));
}

void test_Mat9_invert() {
    Mat9 mat1, mat2, out, mat2_inv, res;

    mat9_set_translate(mat1, 4, 5);
    mat9_set_scale(mat2, 7, 8);

    mat9_product(mat1, mat2, out);

    mat2_inv = mat2.invert();
    mat9_product(out, mat2_inv, res);

    assert(mat1 == res);
}

void test_Mat9_operator_prod() {
    Mat9 mat1, mat2, out;
    mat9_set_translate(mat1, 4, 5);
    mat9_set_scale(mat2, 7, 8);
    mat9_product(mat1, mat2, out);

    assert(out == mat1 * mat2);

    mat1 *= mat2;
    assert(out == mat1);
}

void test_Mat9_operator_prod_scalar() {

    Mat9 mat;
    mat9_set_scale(mat, 4, 5);
    mat9_product(4, mat);

    Mat9 mat2 = Mat9::scale_matrix(4, 5);
    assert(mat == mat2 * 4);
    assert(mat == 4 * mat2);

    mat2 *= 4;
    assert(mat == mat2);
}

void test_Mat9_operator_sum() {
    Mat9 mat1, mat2, out;
    mat9_set_translate(mat1, 4, 5);
    mat9_set_scale(mat2, 7, 8);
    out = mat2;
    mat9_sum(mat1, out);

    assert(out == mat1 + mat2);

    mat1 += mat2;
    assert(out == mat1);
}

#define TEST(x) \
    fprintf(stderr, "Start test " #x "... "); \
    x(); \
    fprintf(stderr, "OK\n");

int main(int argc, char **argv) {
    TEST(test_mat9_set_identity);
    TEST(test_mat9_set_translate);
    TEST(test_mat9_set_scale);
    TEST(test_mat9_sum);
    TEST(test_mat9_product_scalar);
    TEST(test_mat9_product);
    TEST(test_mat9_invert);

    TEST(test_Mat9_access);
    TEST(test_Mat9_set);
    TEST(test_Mat9_ctor);
    TEST(test_Mat9_eq);
    TEST(test_Mat9_invert);

    TEST(test_Mat9_set_identity);
    TEST(test_Mat9_set_translate);
    TEST(test_Mat9_set_scale);

    TEST(test_Mat9_identity_matrix);
    TEST(test_Mat9_translate_matrix);
    TEST(test_Mat9_scale_matrix);

    TEST(test_Mat9_operator_prod);
    TEST(test_Mat9_operator_prod_scalar);
    TEST(test_Mat9_operator_sum);

}

#endif
