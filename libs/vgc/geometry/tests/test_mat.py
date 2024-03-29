#!/usr/bin/python3

# Copyright 2021 The VGC Developers
# See the COPYRIGHT file at the top-level directory of this distribution
# and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest
from math import pi, sqrt
from vgc.geometry import (
    Vec2d, Vec2f, Vec3d, Vec3f, Vec4d, Vec4f,
    Mat2d, Mat2f, Mat3d, Mat3f, Mat4d, Mat4f
)

Mat2Types = [Mat2d, Mat2f]
Mat3Types = [Mat3d, Mat3f]
Mat4Types = [Mat4d, Mat4f]
Mat2Vec2Types = [(Mat2d, Vec2d), (Mat2f, Vec2f)]
Mat3Vec2Types = [(Mat3d, Vec2d), (Mat3f, Vec2f)]
Mat3Vec3Types = [(Mat3d, Vec3d), (Mat3f, Vec3f)]
Mat4Vec2Types = [(Mat4d, Vec2d), (Mat4f, Vec2f)]
Mat4Vec3Types = [(Mat4d, Vec3d), (Mat4f, Vec3f)]
Mat4Vec4Types = [(Mat4d, Vec4d), (Mat4f, Vec4f)]
MatTypes = [(Mat2d, 2), (Mat2f, 2),
            (Mat3d, 3), (Mat3f, 3),
            (Mat4d, 4), (Mat4f, 4)]


class TestMat(unittest.TestCase):

    def testDefaultConstructor(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            for i, j in Mat.indices:
                self.assertEqual(m[i][j], 0)

    def testInitializingConstructor(self):
        for Mat in Mat2Types:
            m = Mat(1, 2,
                    3, 4.5)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[1][0], 3)
            self.assertEqual(m[1][1], 4.5)
        for Mat in Mat3Types:
            m = Mat(1, 2, 3,
                    4, 5, 6,
                    7, 8, 9)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[0][2], 3)
            self.assertEqual(m[1][0], 4)
            self.assertEqual(m[1][1], 5)
            self.assertEqual(m[1][2], 6)
            self.assertEqual(m[2][0], 7)
            self.assertEqual(m[2][1], 8)
            self.assertEqual(m[2][2], 9)

        for Mat in Mat4Types:
            m = Mat(1,  2,  3,  4,
                    5,  6,  7,  8,
                    9,  10, 11, 12,
                    13, 14, 15, 16)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[0][2], 3)
            self.assertEqual(m[0][3], 4)
            self.assertEqual(m[1][0], 5)
            self.assertEqual(m[1][1], 6)
            self.assertEqual(m[1][2], 7)
            self.assertEqual(m[1][3], 8)
            self.assertEqual(m[2][0], 9)
            self.assertEqual(m[2][1], 10)
            self.assertEqual(m[2][2], 11)
            self.assertEqual(m[2][3], 12)
            self.assertEqual(m[3][0], 13)
            self.assertEqual(m[3][1], 14)
            self.assertEqual(m[3][2], 15)
            self.assertEqual(m[3][3], 16)

    def testDiagonalConstructor(self):
        for (Mat, dim) in MatTypes:
            m = Mat(42)
            for i, j in Mat.indices:
                if i == j:
                    self.assertEqual(m[i][j], 42)
                else:
                    self.assertEqual(m[i][j], 0)

    def testFromRows(self):
        for (Mat, Vec) in Mat2Vec2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat((1, 2),
                     (3, 4))
            m3 = Mat(Vec(1, 2),
                     Vec(3, 4))
            self.assertEqual(m1, m2)
            self.assertEqual(m1, m3)
        for (Mat, Vec) in Mat3Vec3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat((1, 2, 3),
                     (4, 5, 6),
                     (7, 8, 9))
            m3 = Mat(Vec(1, 2, 3),
                     Vec(4, 5, 6),
                     Vec(7, 8, 9))
            self.assertEqual(m1, m2)
            self.assertEqual(m1, m3)
        for (Mat, Vec) in Mat4Vec4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat((1,  2,  3,  4),
                     (5,  6,  7,  8),
                     (9,  10, 11, 12),
                     (13, 14, 15, 16))
            m3 = Mat(Vec(1,  2,  3,  4),
                     Vec(5,  6,  7,  8),
                     Vec(9,  10, 11, 12),
                     Vec(13, 14, 15, 16))
            self.assertEqual(m1, m2)
            self.assertEqual(m1, m3)

    def testFromTupleTuple(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat(((1, 2),
                      (3, 4)))
            self.assertEqual(m1, m2)
        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat(((1, 2, 3),
                      (4, 5, 6),
                      (7, 8, 9)))
            self.assertEqual(m1, m2)
        for Mat in Mat4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat(((1,  2,  3,  4),
                      (5,  6,  7,  8),
                      (9,  10, 11, 12),
                      (13, 14, 15, 16)))
            self.assertEqual(m1, m2)

    def testFromTupleList(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat([(1, 2),
                      (3, 4)])
            self.assertEqual(m1, m2)
        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat([(1, 2, 3),
                      (4, 5, 6),
                      (7, 8, 9)])
            self.assertEqual(m1, m2)
        for Mat in Mat4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat([(1,  2,  3,  4),
                      (5,  6,  7,  8),
                      (9,  10, 11, 12),
                      (13, 14, 15, 16)])
            self.assertEqual(m1, m2)

    def testFromListTuple(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat(([1, 2],
                      [3, 4]))
            self.assertEqual(m1, m2)
        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat(([1, 2, 3],
                      [4, 5, 6],
                      [7, 8, 9]))
            self.assertEqual(m1, m2)
        for Mat in Mat4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat(([1,  2,  3,  4],
                      [5,  6,  7,  8],
                      [9,  10, 11, 12],
                      [13, 14, 15, 16]))
            self.assertEqual(m1, m2)

    def testFromListList(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat([[1, 2],
                      [3, 4]])
            self.assertEqual(m1, m2)
        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat([[1, 2, 3],
                      [4, 5, 6],
                      [7, 8, 9]])
            self.assertEqual(m1, m2)
        for Mat in Mat4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat([[1,  2,  3,  4],
                      [5,  6,  7,  8],
                      [9,  10, 11, 12],
                      [13, 14, 15, 16]])
            self.assertEqual(m1, m2)

    def testFromFlatList(self):
        for Mat in Mat2Types:
            m1 = Mat(0, 1,
                     2, 3)
            m2 = Mat([2*i+j for i,j in Mat.indices])
            self.assertEqual(m1, m2)
        for Mat in Mat3Types:
            m1 = Mat(0, 1, 2,
                     2, 3, 4,
                     4, 5, 6)
            m2 = Mat([2*i+j for i,j in Mat.indices])
            self.assertEqual(m1, m2)
        for Mat in Mat4Types:
            m1 = Mat(0, 1, 2, 3,
                     2, 3, 4, 5,
                     4, 5, 6, 7,
                     6, 7, 8, 9)
            m2 = Mat([2*i+j for i,j in Mat.indices])
            self.assertEqual(m1, m2)

    # See wraps/vec.h: "Implicit conversion from string is not allowed in this context"
    #
    def testFromStringList(self):
        for Mat in Mat2Types:
            with self.assertRaises(ValueError):
                m = Mat(['(1, 2)',
                         '(3, 4)'])
        for Mat in Mat3Types:
            with self.assertRaises(ValueError):
                m = Mat(['(1, 2, 3)',
                         '(4, 5, 6)',
                         '(7, 8, 9)'])
        for Mat in Mat4Types:
            with self.assertRaises(ValueError):
                m = Mat(['(1, 2, 3, 4)',
                         '(5, 6, 7, 8)',
                         '(9, 10, 11, 12)',
                         '(13, 14, 15, 16)'])

    def testCopyByReference(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            n = m
            self.assertIs(m, n)
            self.assertEqual(m, n)
            n[0][0] = 15
            self.assertEqual(n[0][0], 15)
            self.assertEqual(m[0][0], 15)
            self.assertEqual(m, n)

    def testCopyByValue(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            n = Mat(m)
            self.assertIsNot(m, n)
            self.assertEqual(m, n)
            n[0][0] = 15
            self.assertEqual(n[0][0], 15)
            self.assertEqual(m[0][0], 0)
            self.assertNotEqual(m, n)

    def testFromLowerDimension(self):
        for Mat2 in Mat2Types:
            for Mat3 in Mat3Types:
                m2 = Mat2(2, 3,
                          4, 5)

                m3 = Mat3(2, 3, 0,
                          4, 5, 0,
                          0, 0, 1)
                self.assertEqual(Mat3.fromLinear(m2), m3)

                m3 = Mat3(2, 0, 3,
                          0, 1, 0,
                          4, 0, 5)
                self.assertEqual(Mat3.fromTransform(m2), m3)

            for Mat4 in Mat4Types:
                m2 = Mat2(2, 3,
                          4, 5)

                m4 = Mat4(2, 3, 0, 0,
                          4, 5, 0, 0,
                          0, 0, 1, 0,
                          0, 0, 0, 1)
                self.assertEqual(Mat4.fromLinear(m2), m4)

                m4 = Mat4(2, 0, 0, 3,
                          0, 1, 0, 0,
                          0, 0, 1, 0,
                          4, 0, 0, 5)
                self.assertEqual(Mat4.fromTransform(m2), m4)

        for Mat3 in Mat3Types:
            for Mat4 in Mat4Types:
                m3 = Mat3(2, 3, 4,
                          5, 6, 7,
                          8, 9, 10)

                m4 = Mat4(2, 3, 4, 0,
                          5, 6, 7, 0,
                          8, 9, 10, 0,
                          0, 0, 0, 1)
                self.assertEqual(Mat4.fromLinear(m3), m4)

                m4 = Mat4(2, 3, 0, 4,
                          5, 6, 0, 7,
                          0, 0, 1, 0,
                          8, 9, 0, 10)
                self.assertEqual(Mat4.fromTransform(m3), m4)

    def testSetElements(self):
        for Mat in Mat2Types:
            m = Mat()
            m.setElements(
                    1, 2,
                    3, 4.5)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[1][0], 3)
            self.assertEqual(m[1][1], 4.5)

        for Mat in Mat3Types:
            m = Mat()
            m.setElements(
                    1, 2, 3,
                    4, 5, 6,
                    7, 8, 9)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[0][2], 3)
            self.assertEqual(m[1][0], 4)
            self.assertEqual(m[1][1], 5)
            self.assertEqual(m[1][2], 6)
            self.assertEqual(m[2][0], 7)
            self.assertEqual(m[2][1], 8)
            self.assertEqual(m[2][2], 9)

        for Mat in Mat4Types:
            m = Mat()
            m.setElements(
                    1,  2,  3,  4,
                    5,  6,  7,  8,
                    9,  10, 11, 12,
                    13, 14, 15, 16)
            self.assertEqual(m[0][0], 1)
            self.assertEqual(m[0][1], 2)
            self.assertEqual(m[0][2], 3)
            self.assertEqual(m[0][3], 4)
            self.assertEqual(m[1][0], 5)
            self.assertEqual(m[1][1], 6)
            self.assertEqual(m[1][2], 7)
            self.assertEqual(m[1][3], 8)
            self.assertEqual(m[2][0], 9)
            self.assertEqual(m[2][1], 10)
            self.assertEqual(m[2][2], 11)
            self.assertEqual(m[2][3], 12)
            self.assertEqual(m[3][0], 13)
            self.assertEqual(m[3][1], 14)
            self.assertEqual(m[3][2], 15)
            self.assertEqual(m[3][3], 16)

    def testSetToDiagonal(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            m.setToDiagonal(42)
            for i, j in Mat.indices:
                if i == j:
                    self.assertEqual(m[i][j], 42)
                else:
                    self.assertEqual(m[i][j], 0)

    def testSetToZero(self):
        for (Mat, dim) in MatTypes:
            if dim == 2:
                m = Mat(1, 2,
                        3, 4.5)
            elif dim == 3:
                m = Mat(1, 2, 3,
                        4, 5, 6,
                        7, 8, 9)
            elif dim == 4:
                m = Mat(1,  2,  3,  4,
                        5,  6,  7,  8,
                        9,  10, 11, 12,
                        13, 14, 15, 16)
            m.setToZero()
            self.assertEqual(m, Mat())

    def testSetToIdentity(self):
        for (Mat, dim) in MatTypes:
            if dim == 2:
                m = Mat(1, 2,
                        3, 4.5)
            elif dim == 3:
                m = Mat(1, 2, 3,
                        4, 5, 6,
                        7, 8, 9)
            elif dim == 4:
                m = Mat(1,  2,  3,  4,
                        5,  6,  7,  8,
                        9,  10, 11, 12,
                        13, 14, 15, 16)
            m.setToIdentity()
            self.assertEqual(m, Mat(1))

    def testIdentity(self):
        for (Mat, dim) in MatTypes:
            m = Mat.identity
            self.assertEqual(m, Mat(1))

            m[0][0] = 42
            self.assertNotEqual(m, Mat(1))
            self.assertEqual(Mat.identity, Mat(1))

            m = 42
            self.assertNotEqual(m, Mat(1))
            self.assertEqual(Mat.identity, Mat(1))

            Mat.identity[0][0] = 42   # => actually mutates a temporary
            self.assertEqual(Mat.identity, Mat(1))

            with self.assertRaises(AttributeError):
                Mat.identity = 42

    def testBracketOperator(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            m[0][0] = 42
            m[0][0] += 42
            self.assertEqual(m[0][0], 84)
            with self.assertRaises(IndexError):
                m[-1][0] = 0
            with self.assertRaises(IndexError):
                m[4][0] = 0
            with self.assertRaises(IndexError):
                m[0][-1] = 0
            with self.assertRaises(IndexError):
                m[0][4] = 0
            with self.assertRaises(TypeError):
                m[0] = 42
                # Note: it might be nice though to be able to do:
                # m[0] = [1, 2, 3, 4]   (assuming a 4x4 matrix)

            # Test row keeps alive matrix
            row = m[0]
            row[0] = 12
            self.assertEqual(row[0], 12)
            self.assertEqual(m[0][0], 12)
            m = None
            self.assertEqual(row[0], 12)

    def testIndices(self):
        for (Mat, dim) in MatTypes:
            m = Mat()
            for i, j in Mat.indices:  # Without parentheses
                m[i][j] = j + i*dim
            for i in range(dim):
                for j in range(dim):
                    self.assertEqual(m[i][j], j + i*dim)
            for (i, j) in Mat.indices:  # With parentheses
                m[i][j] = 1 + j + i*dim
            for i in range(dim):
                for j in range(dim):
                    self.assertEqual(m[i][j], 1 + j + i*dim)

    def testArithmeticOperators(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2, 3, 4)
            m2 = Mat(10, 20, 30, 40)
            m3 = m1 + m2
            self.assertEqual(m1, Mat(1, 2, 3, 4))
            self.assertEqual(m2, Mat(10, 20, 30, 40))
            self.assertEqual(m3, Mat(11, 22, 33, 44))
            m3 += m2
            self.assertEqual(m2, Mat(10, 20, 30, 40))
            self.assertEqual(m3, Mat(21, 42, 63, 84))
            m4 = +m1
            self.assertEqual(m1, Mat(1, 2, 3, 4))
            self.assertEqual(m4, Mat(1, 2, 3, 4))
            m3 = m2 - m1
            self.assertEqual(m2, Mat(10, 20, 30, 40))
            self.assertEqual(m1, Mat(1, 2, 3, 4))
            self.assertEqual(m3, Mat(9, 18, 27, 36))
            m3 -= m2
            self.assertEqual(m2, Mat(10, 20, 30, 40))
            self.assertEqual(m3, Mat(-1, -2, -3, -4))
            m4 = -m2
            self.assertEqual(m2, Mat(10, 20, 30, 40))
            self.assertEqual(m4, Mat(-10, -20, -30, -40))
            m3 = 3 * m1 * 2
            self.assertEqual(m1, Mat(1, 2, 3, 4))
            self.assertEqual(m3, Mat(6, 12, 18, 24))
            m3 *= 10
            self.assertEqual(m3, Mat(60, 120, 180, 240))
            m3 /= 10
            self.assertEqual(m3, Mat(6, 12, 18, 24))
            m4 = m3 / 2
            self.assertEqual(m4, Mat(3, 6, 9, 12))

        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3, 4, 5, 6, 7, 8, 9)
            m2 = Mat(10, 20, 30, 40, 50, 60, 70, 80, 90)
            m3 = m1 + m2
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9))
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90))
            self.assertEqual(m3, Mat(11, 22, 33, 44, 55, 66, 77, 88, 99))
            m3 += m2
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90))
            self.assertEqual(m3, Mat(21, 42, 63, 84, 105, 126, 147, 168, 189))
            m4 = +m1
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9))
            self.assertEqual(m4, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9))
            m3 = m2 - m1
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90))
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9))
            self.assertEqual(m3, Mat(9, 18, 27, 36, 45, 54, 63, 72, 81))
            m3 -= m2
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90))
            self.assertEqual(m3, Mat(-1, -2, -3, -4, -5, -6, -7, -8, -9))
            m4 = -m2
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90))
            self.assertEqual(m4, Mat(-10, -20, -30, -40, -50, -60, -70, -80, -90))
            m3 = 3 * m1 * 2
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9))
            self.assertEqual(m3, Mat(6, 12, 18, 24, 30, 36, 42, 48, 54))
            m3 *= 10
            self.assertEqual(m3, Mat(60, 120, 180, 240, 300, 360, 420, 480, 540))
            m3 /= 10
            self.assertEqual(m3, Mat(6, 12, 18, 24, 30, 36, 42, 48, 54))
            m4 = m3 / 2
            self.assertEqual(m4, Mat(3, 6, 9, 12, 15, 18, 21, 24, 27))

        for Mat in Mat4Types:
            m1 = Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
            m2 = Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160)
            m3 = m1 + m2
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160))
            self.assertEqual(m3, Mat(11, 22, 33, 44, 55, 66, 77, 88, 99, 110, 121, 132, 143, 154, 165, 176))
            m3 = Mat(m1)
            m3 += m2
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160))
            self.assertEqual(m3, Mat(11, 22, 33, 44, 55, 66, 77, 88, 99, 110, 121, 132, 143, 154, 165, 176))
            m4 = +m1
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            self.assertEqual(m4, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            m3 = m2 - m1
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160))
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            self.assertEqual(m3, Mat(9, 18, 27, 36, 45, 54, 63, 72, 81, 90, 99, 108, 117, 126, 135, 144))
            m3 -= m2
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160))
            self.assertEqual(m3, Mat(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16))
            m4 = -m2
            self.assertEqual(m2, Mat(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160))
            self.assertEqual(m4, Mat(-10, -20, -30, -40, -50, -60, -70, -80, -90, -100, -110, -120, -130, -140, -150, -160))
            m3 = 3 * m1 * 2
            self.assertEqual(m1, Mat(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
            self.assertEqual(m3, Mat(6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96))
            m3 *= 10
            self.assertEqual(m3, Mat(60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960))
            m3 /= 10
            self.assertEqual(m3, Mat(6, 12, 18, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96))
            m4 = m3 / 2
            self.assertEqual(m4, Mat(3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48))

    def testMatMatMultiplication(self):
        for Mat in Mat2Types:
            m1 = Mat(1, 2,
                     3, 4)
            m2 = Mat(30, 40,
                     10, 20)
            m3 = Mat(50, 80,
                     130, 200)
            m4 = Mat(150, 220,
                     70, 100)
            m5 = m1 * m2
            m6 = m2 * m1
            m7 = Mat(m1)
            m7 *= m2
            m8 = Mat(m2)
            m8 *= m1
            self.assertEqual(m5, m3)
            self.assertEqual(m6, m4)
            self.assertEqual(m7, m3)
            self.assertEqual(m8, m4)

        for Mat in Mat3Types:
            m1 = Mat(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            m2 = Mat(40, 50, 60,
                     10, 20, 30,
                     70, 80, 90)
            m3 = Mat(270, 330, 390,
                     630, 780, 930,
                     990, 1230, 1470)
            m4 = Mat(660, 810, 960,
                     300, 360, 420,
                     1020, 1260, 1500)
            m5 = m1 * m2
            m6 = m2 * m1
            m7 = Mat(m1)
            m7 *= m2
            m8 = Mat(m2)
            m8 *= m1
            self.assertEqual(m5, m3)
            self.assertEqual(m6, m4)
            self.assertEqual(m7, m3)
            self.assertEqual(m8, m4)

        for Mat in Mat4Types:
            m1 = Mat(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9,  10, 11, 12,
                     13, 14, 15, 16)
            m2 = Mat(50,  60,  70,  80,
                     10,  20,  30,  40,
                     90,  100, 110, 120,
                     130, 140, 150, 160)
            m3 = Mat(860,  960,  1060, 1160,
                     1980, 2240, 2500, 2760,
                     3100, 3520, 3940, 4360,
                     4220, 4800, 5380, 5960)
            m4 = Mat(2020, 2280, 2540, 2800,
                     900,  1000, 1100, 1200,
                     3140, 3560, 3980, 4400,
                     4260, 4840, 5420, 6000)
            m5 = m1 * m2
            m6 = m2 * m1
            m7 = Mat(m1)
            m7 *= m2
            m8 = Mat(m2)
            m8 *= m1
            self.assertEqual(m5, m3)
            self.assertEqual(m6, m4)
            self.assertEqual(m7, m3)
            self.assertEqual(m8, m4)

    def testTransformAffine(self):
        for (Mat3, Vec2) in Mat3Vec2Types:
            m = Mat3(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            v1 = Vec2(10, 20)
            v2 = Vec2(53, 146)
            v3 = m.transformAffine(v1)
            self.assertEqual(v3, v2)

        for (Mat4, Vec2) in Mat4Vec2Types:
            m = Mat4(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9, 10,  11, 12,
                     13, 14, 15, 16)
            v1 = Vec2(10, 20)
            v2 = Vec2(54, 178)
            v3 = m.transformAffine(v1)
            self.assertEqual(v3, v2)

        for (Mat4, Vec3) in Mat4Vec3Types:
            m = Mat4(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9, 10,  11, 12,
                     13, 14, 15, 16)
            v1 = Vec3(10, 20, 30)
            v2 = Vec3(144, 388, 632)
            v3 = m.transformAffine(v1)
            self.assertEqual(v3, v2)

    def testTransformLinear(self):
        for Mat2 in Mat2Types:
            m = Mat2(2, 3,
                     4, 5)
            y = m.transformLinear(6)
            self.assertEqual(y, 12)

        for (Mat3, Vec2) in Mat3Vec2Types:
            m = Mat3(1, 2, 3,
                     4, 5, 6,
                     7, 8, 9)
            v1 = Vec2(10, 20)
            v2 = Vec2(50, 140)
            v3 = m.transformLinear(v1)
            self.assertEqual(v3, v2)

        for (Mat4, Vec2) in Mat4Vec2Types:
            m = Mat4(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9, 10,  11, 12,
                     13, 14, 15, 16)
            v1 = Vec2(10, 20)
            v2 = Vec2(50, 170)
            v3 = m.transformLinear(v1)
            self.assertEqual(v3, v2)

        for (Mat4, Vec3) in Mat4Vec3Types:
            m = Mat4(1,  2,  3,  4,
                     5,  6,  7,  8,
                     9, 10,  11, 12,
                     13, 14, 15, 16)
            v1 = Vec3(10, 20, 30)
            v2 = Vec3(140, 380, 620)
            v3 = m.transformLinear(v1)
            self.assertEqual(v3, v2)

    def testInvertedScale(self):
        for Mat in Mat3Types:
            m = Mat(1, 2, 3,
                    4, 5, 6,
                    7, 8, 9)
            with self.assertRaises(ValueError):
                m3 = m.inverse()
            m = Mat(+1,  2,  4,
                    -4, -3, -2,
                    +5, -1,  3)
            m2 = (1.0 / 69) * Mat(-11, -10, 8,
                                  +2,  -17, -14,
                                  +19,  11, 5)
            m3 = m.inverse()
            self.assertEqual(m2, m3)

        for Mat in Mat4Types:
            m = Mat(+1,  2,  3,   4,
                    -1, -3, -12, -4,
                    +7,  8,  9,   5,
                    -6, -5, -8,  -9)
            m2 = (1.0 / 589) * Mat(-365, 17, -13, -177,
                                   +347, 50, 135, 207,
                                   -104, -71, -15, -23,
                                   +143, 24, -53, -42)
            m3 = m.inverse()
            self.assertEqual(m2, m3)

    def testTranslate(self):
        for (Mat, Vec2) in Mat3Vec2Types + Mat4Vec2Types:
            v = Vec2(1, 2)
            m1 = Mat.identity.translate(10)
            m2 = Mat.identity
            m2.translate(10)
            self.assertEqual(m1.transform(v), Vec2(11, 2))
            self.assertEqual(m2.transform(v), Vec2(11, 2))
            m1 = Mat.identity.translate(10, 20)
            m2 = Mat.identity
            m2.translate(10, 20)
            self.assertEqual(m1.transform(v), Vec2(11, 22))
            self.assertEqual(m2.transform(v), Vec2(11, 22))

        for (Mat3, Vec2) in Mat3Vec2Types:
            v = Vec2(1, 2)
            m1 = Mat3.identity.translate(Vec2(10, 20))
            self.assertEqual(m1.transform(v), Vec2(11, 22))

        for (Mat4, Vec2) in Mat4Vec2Types:
            v = Vec2(1, 2)
            m1 = Mat4.identity.translate(10, 20, 30)
            m2 = Mat4.identity
            m2.translate(10, 20, 30)
            self.assertEqual(m1.transform(v), Vec2(11, 22))
            self.assertEqual(m2.transform(v), Vec2(11, 22))

    def testRotate(self):
        for (Mat, Vec2) in Mat3Vec2Types + Mat4Vec2Types:
            v = Vec2(1, 2)
            m1 = Mat.identity.rotate(pi/2)
            m2 = Mat.identity
            m2.rotate(pi/2)
            self.assertEqual(m1.transform(v), Vec2(-2, 1))
            self.assertEqual(m2.transform(v), Vec2(-2, 1))
            v = Vec2(2, 0)
            m1 = Mat.identity.rotate(pi/3)
            m2 = Mat.identity
            m2.rotate(pi/3)
            v2 = Vec2(1, sqrt(3))
            absTol = 1.0e-6
            self.assertTrue(v2.allNear(m1.transform(v), absTol))
            self.assertTrue(v2.allNear(m2.transform(v), absTol))

    def testScale(self):
        for (Mat, Vec2) in Mat3Vec2Types + Mat4Vec2Types:
            v = Vec2(2, 3)
            m1 = Mat.identity.scale(20)
            m2 = Mat.identity
            m2.scale(20)
            self.assertEqual(m1.transform(v), Vec2(40, 60))
            self.assertEqual(m2.transform(v), Vec2(40, 60))
            m1 = Mat.identity.scale(20, 30)
            m2 = Mat.identity
            m2.scale(20, 30)
            self.assertTrue(m1.transform(v), Vec2(40, 90))
            self.assertEqual(m2.transform(v), Vec2(40, 90))

        for (Mat3, Vec2) in Mat3Vec2Types:
            v = Vec2(2, 3)
            m1 = Mat3.identity.scale(Vec2(20, 30))
            self.assertTrue(m1.transform(v), Vec2(40, 90))

        for (Mat4, Vec2) in Mat4Vec2Types:
            v = Vec2(2, 3)
            m1 = Mat4.identity.scale(20, 30, 40)
            m2 = Mat4.identity
            m2.scale(20, 30, 40)
            self.assertEqual(m1.transform(v), Vec2(40, 90))
            self.assertEqual(m2.transform(v), Vec2(40, 90))

    def testComparisonOperators(self):
        for (Mat, dim) in MatTypes:
            m1 = Mat.identity
            m2 = Mat(1)
            m3 = Mat(2)
            self.assertTrue(m1 == m2)
            self.assertTrue(m1 != m3)
            self.assertFalse(m1 != m2)
            self.assertFalse(m1 == m3)

    def testToString(self):
        m = Mat2d(1, 2, 3, 4)
        self.assertEqual(str(m), "((1, 2), (3, 4))")
        self.assertEqual(repr(m), "vgc.geometry.Mat2d((1, 2), (3, 4))")
        for Mat in Mat2Types:
            # Setup a French locale (if available on this system) to check that even
            # when the decimal point is ',' according to the locale, numbers are
            # still printed with '.' as decimal point.
            #
            try:
                locale.setlocale(locale.LC_ALL, 'fr_FR.UTF8')
            except:
                pass
            m = Mat(1, 2,
                    3, 4.5)
            s = "((1, 2), (3, 4.5))"
            self.assertEqual(str(m), s)
            self.assertEqual(repr(m), "vgc.geometry." + Mat.__name__ + s)
        for Mat in Mat3Types:
            m = Mat(1, 2, 3,
                    4, 5, 6,
                    7, 8, 9)
            s = "((1, 2, 3), (4, 5, 6), (7, 8, 9))"
            self.assertEqual(str(m), s)
            self.assertEqual(repr(m), "vgc.geometry." + Mat.__name__ + s)
        for Mat in Mat4Types:
            m = Mat(1,  2,  3,  4,
                    5,  6,  7,  8,
                    9,  10, 11, 12,
                    13, 14, 15, 16)
            s = "((1, 2, 3, 4), (5, 6, 7, 8), (9, 10, 11, 12), (13, 14, 15, 16))"
            self.assertEqual(str(m), s)
            self.assertEqual(repr(m), "vgc.geometry." + Mat.__name__ + s)

    def testParse(self):
        for Mat in Mat2Types:
            m = Mat(1, 2,
                    3, 4.5)
            s = "  ( (1, 2)  ,\n(3, 4.5)) " # test various formatting variants
            self.assertEqual(Mat(s), m)
        for Mat in Mat3Types:
            m = Mat(1, 2, 3,
                    4, 5, 6,
                    7, 8, 9)
            s = "((1, 2, 3), (4, 5, 6), (7, 8, 9))"
            self.assertEqual(Mat(s), m)
        for Mat in Mat4Types:
            m = Mat(1,  2,  3,  4,
                    5,  6,  7,  8,
                    9,  10, 11, 12,
                    13, 14, 15, 16)
            s = "((1, 2, 3, 4), (5, 6, 7, 8), (9, 10, 11, 12), (13, 14, 15, 16))"
            self.assertEqual(Mat(s), m)

if __name__ == '__main__':
    unittest.main()
