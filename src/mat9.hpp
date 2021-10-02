/*
 * Copyright (c) 2020, 2021 Goffredo Baroncelli
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

#pragma once

#include <cassert>

struct Mat9 {
    float coeff[9];
    float & operator[](int idx) {
        assert(idx >= 0 && idx < 9);
        return coeff[idx];
    }
    float operator[](int idx) const {
        assert(idx >= 0 && idx < 9);
        return coeff[idx];
    }
    void set (float x0, float x1, float x2, float x3, float x4, float x5,
        float x6, float x7, float x8) {
            coeff[0] = x0; coeff[1] = x1; coeff[2] = x2; coeff[3] = x3;
            coeff[4] = x4; coeff[5] = x5; coeff[6] = x6; coeff[7] = x7;
            coeff[8] = x8;
    }
    Mat9(float x0, float x1, float x2, float x3, float x4, float x5, float x6,
        float x7, float x8) {
            set(x0, x1, x2, x3, x4, x5, x6, x7, x8);
    }
    Mat9() {}
};

void mat9_set_identity(Mat9 &m);
void mat9_print(const Mat9 &m);
void mat9_sum(const Mat9 &m1, Mat9 &m2);
void mat9_product(const float c, Mat9 &m1);
void mat9_product(const Mat9 &m1, const Mat9 &m2, Mat9 &m3);
void mat9_invert(const Mat9 &m, Mat9 &minv);
