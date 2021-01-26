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
    memcpy(m.coeff, id, sizeof(float) * 9);
}
