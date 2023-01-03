/**
 * \file
 * \copyright
 * Copyright (c) 2012-2023, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

namespace NumLib
{
template <class T_X, class T_N>
void ShapePyra13::computeShapeFunction(const T_X& x, T_N& N)
{
    const double r = x[0];
    const double s = x[1];
    const double t = x[2];

    N[0] = -0.0625 * (1.0 - r) * (1.0 - s) * (1.0 - t) *
           (4.0 + 3.0 * r + 3.0 * s + 2.0 * r * s + 2.0 * t + r * t + s * t +
            2.0 * r * s * t);
    N[1] = -0.0625 * (1.0 + r) * (1.0 - s) * (1.0 - t) *
           (4.0 - 3.0 * r + 3.0 * s - 2.0 * r * s + 2.0 * t - r * t + s * t -
            2.0 * r * s * t);
    N[2] = -0.0625 * (1.0 + r) * (1.0 + s) * (1.0 - t) *
           (4.0 - 3.0 * r - 3.0 * s + 2.0 * r * s + 2.0 * t - r * t - s * t +
            2.0 * r * s * t);
    N[3] = -0.0625 * (1.0 - r) * (1.0 + s) * (1.0 - t) *
           (4.0 + 3.0 * r - 3.0 * s - 2.0 * r * s + 2.0 * t + r * t - s * t -
            2.0 * r * s * t);
    N[4] = 0.5 * t * (1.0 + t);
    N[5] = 0.125 * (1.0 - r * r) * (1.0 - s) * (1.0 - t) * (2.0 + s + s * t);
    N[6] = 0.125 * (1.0 + r) * (1.0 - s * s) * (1.0 - t) * (2.0 - r - r * t);
    N[7] = 0.125 * (1.0 - r * r) * (1.0 + s) * (1.0 - t) * (2.0 - s - s * t);
    N[8] = 0.125 * (1.0 - r) * (1.0 - s * s) * (1.0 - t) * (2.0 + r + r * t);
    N[9] = 0.25 * (1.0 - r) * (1.0 - s) * (1.0 - t * t);
    N[10] = 0.25 * (1.0 + r) * (1.0 - s) * (1.0 - t * t);
    N[11] = 0.25 * (1.0 + r) * (1.0 + s) * (1.0 - t * t);
    N[12] = 0.25 * (1.0 - r) * (1.0 + s) * (1.0 - t * t);
}

template <class T_X, class T_N>
void ShapePyra13::computeGradShapeFunction(const T_X& x, T_N& dN)
{
    const double r = x[0];
    const double s = x[1];
    const double t = x[2];
    //---dN/dr
    dN[0] = 0.0625 * (1.0 - s) * (1.0 - t) *
            (1.0 + 6.0 * r + s + 4.0 * r * s + t + 2.0 * r * t - s * t +
             4.0 * r * s * t);
    dN[1] = -0.0625 * (1.0 - s) * (1.0 - t) *
            (1.0 - 6.0 * r + s - 4.0 * r * s + t - 2.0 * r * t - s * t -
             4.0 * r * s * t);
    dN[2] = -0.0625 * (1.0 + s) * (1.0 - t) *
            (1.0 - 6.0 * r - s + 4.0 * r * s + t - 2.0 * r * t + s * t +
             4.0 * r * s * t);
    dN[3] = 0.0625 * (1.0 + s) * (1.0 - t) *
            (1.0 + 6.0 * r - s - 4.0 * r * s + t + 2.0 * r * t + s * t -
             4.0 * r * s * t);
    dN[4] = 0.0;
    dN[5] = -0.25 * r * (1.0 - s) * (1.0 - t) * (2.0 + s + s * t);
    dN[6] = 0.125 * (1.0 - s * s) * (1.0 - t) * (1.0 - 2.0 * r - t - 2 * r * t);
    dN[7] = -0.25 * r * (1.0 + s) * (1.0 - t) * (2.0 - s - s * t);
    dN[8] =
        -0.125 * (1.0 - s * s) * (1.0 - t) * (1.0 + 2.0 * r - t + 2 * r * t);
    dN[9] = -0.25 * (1.0 - s) * (1.0 - t * t);
    dN[10] = 0.25 * (1.0 - s) * (1.0 - t * t);
    dN[11] = 0.25 * (1.0 + s) * (1.0 - t * t);
    dN[12] = -0.25 * (1.0 + s) * (1.0 - t * t);

    //---dN/ds
    dN[13] = 0.0625 * (1.0 - r) * (1.0 - t) *
             (1.0 + r + 6.0 * s + 4.0 * r * s + t - r * t + 2.0 * s * t +
              4.0 * r * s * t);
    dN[14] = 0.0625 * (1.0 + r) * (1.0 - t) *
             (1.0 - r + 6.0 * s - 4.0 * r * s + t + r * t + 2.0 * s * t -
              4.0 * r * s * t);
    dN[15] = -0.0625 * (1.0 + r) * (1.0 - t) *
             (1.0 - r - 6.0 * s + 4.0 * r * s + t + r * t - 2.0 * s * t +
              4.0 * r * s * t);
    dN[16] = -0.0625 * (1.0 - r) * (1.0 - t) *
             (1.0 + r - 6.0 * s - 4.0 * r * s + t - r * t - 2.0 * s * t -
              4.0 * r * s * t);
    dN[17] = 0.0;
    dN[18] =
        -0.125 * (1.0 - r * r) * (1.0 - t) * (1.0 + 2.0 * s - t + 2.0 * s * t);
    dN[19] = -0.25 * (1.0 + r) * s * (1.0 - t) * (2.0 - r - r * t);
    dN[20] =
        0.125 * (1.0 - r * r) * (1.0 - t) * (1.0 - 2.0 * s - t - 2.0 * s * t);
    dN[21] = -0.25 * (1.0 - r) * s * (1.0 - t) * (2.0 + r + r * t);
    dN[22] = -0.25 * (1.0 - r) * (1.0 - t * t);
    dN[23] = -0.25 * (1.0 + r) * (1.0 - t * t);
    dN[24] = 0.25 * (1.0 + r) * (1.0 - t * t);
    dN[25] = 0.25 * (1.0 - r) * (1.0 - t * t);

    //---dN/dt
    dN[26] = 0.125 * (1.0 - r) * (1.0 - s) *
             (1.0 + r + s + 2.0 * t + r * t + s * t + 2.0 * r * s * t);
    dN[27] = 0.125 * (1.0 + r) * (1.0 - s) *
             (1.0 - r + s + 2.0 * t - r * t + s * t - 2.0 * r * s * t);
    dN[28] = 0.125 * (1.0 + r) * (1.0 + s) *
             (1.0 - r - s + 2.0 * t - r * t - s * t + 2.0 * r * s * t);
    dN[29] = 0.125 * (1.0 - r) * (1.0 + s) *
             (1.0 + r - s + 2.0 * t + r * t - s * t - 2.0 * r * s * t);
    dN[30] = 0.5 + t;
    dN[31] = -0.25 * (1.0 - r * r) * (1.0 - s) * (1.0 + s * t);
    dN[32] = -0.25 * (1.0 + r) * (1.0 - s * s) * (1.0 - r * t);
    dN[33] = -0.25 * (1.0 - r * r) * (1.0 + s) * (1.0 - s * t);
    dN[34] = -0.25 * (1.0 - r) * (1.0 - s * s) * (1.0 + r * t);
    dN[35] = -0.5 * (1.0 - r) * (1.0 - s) * t;
    dN[36] = -0.5 * (1.0 + r) * (1.0 - s) * t;
    dN[37] = -0.5 * (1.0 + r) * (1.0 + s) * t;
    dN[38] = -0.5 * (1.0 - r) * (1.0 + s) * t;
}

}  // namespace NumLib
