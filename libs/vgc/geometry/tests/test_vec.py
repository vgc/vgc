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

from math import inf, pi
import locale
import unittest

from vgc.geometry import (
    Vec2d, Vec2f, Vec2dArray, Vec2fArray,
    Vec3d, Vec3f, Vec3dArray, Vec3fArray,
    Vec4d, Vec4f, Vec4dArray, Vec4fArray
)

Vec2Types = [Vec2d, Vec2f]
Vec3Types = [Vec3d, Vec3f]
Vec4Types = [Vec4d, Vec4f]

VecTypes = [(Vec2d, 2), (Vec2f, 2),
            (Vec3d, 3), (Vec3f, 3),
            (Vec4d, 4), (Vec4f, 4)]

VecArrayTypes = [(Vec2dArray, Vec2d, 2), (Vec2fArray, Vec2f, 2),
                 (Vec3dArray, Vec3d, 3), (Vec3fArray, Vec3f, 3),
                 (Vec4dArray, Vec4d, 4), (Vec4fArray, Vec4f, 4)]


class TestVec(unittest.TestCase):

    def testDefaultConstructor(self):
        for (Vec, dim) in VecTypes:
            v = Vec()
            for i in range(dim):
                self.assertEqual(v[i], 0)

    def testInitializingConstructor(self):
        for Vec in Vec2Types:
            v = Vec(12.5, 42)
            self.assertEqual(v.x, 12.5)
            self.assertEqual(v.y, 42)
        for Vec in Vec3Types:
            v = Vec(12.5, 42, 12)
            self.assertEqual(v.x, 12.5)
            self.assertEqual(v.y, 42)
            self.assertEqual(v.z, 12)
        for Vec in Vec4Types:
            v = Vec(12.5, 42, 12, 13)
            self.assertEqual(v.x, 12.5)
            self.assertEqual(v.y, 42)
            self.assertEqual(v.z, 12)
            self.assertEqual(v.w, 13)

    def testFromTuple(self):
        for Vec in Vec2Types:
            self.assertEqual(Vec((1, 2)), Vec(1, 2))
            with self.assertRaises(ValueError):
                v = Vec((1, 2, 3))
        for Vec in Vec3Types:
            self.assertEqual(Vec((1, 2, 3)), Vec(1, 2, 3))
            with self.assertRaises(ValueError):
                v = Vec((1, 2, 3, 4))
        for Vec in Vec4Types:
            self.assertEqual(Vec((1, 2, 3, 4)), Vec(1, 2, 3, 4))
            with self.assertRaises(ValueError):
                v = Vec((1, 2, 3))

    def testFromList(self):
        for Vec in Vec2Types:
            self.assertEqual(Vec([1, 2]), Vec(1, 2))
            with self.assertRaises(ValueError):
                v = Vec([1, 2, 3])
        for Vec in Vec3Types:
            self.assertEqual(Vec([1, 2, 3]), Vec(1, 2, 3))
            with self.assertRaises(ValueError):
                v = Vec([1, 2, 3, 4])
        for Vec in Vec4Types:
            self.assertEqual(Vec([1, 2, 3, 4]), Vec(1, 2, 3, 4))
            with self.assertRaises(ValueError):
                v = Vec([1, 2, 3])

    def testCopyByReference(self):
        for (Vec, dim) in VecTypes:
            v1 = Vec()
            v2 = v1
            self.assertIs(v1, v2)
            self.assertEqual(v1, v2)
            v2.x = 15
            self.assertEqual(v1.x, 15)
            self.assertEqual(v1, v2)

    def testCopyByValue(self):
        for (Vec, dim) in VecTypes:
            v1 = Vec()
            v2 = Vec(v1)
            self.assertIsNot(v1, v2)
            self.assertEqual(v1, v2)
            v2.x = 15
            self.assertEqual(v1.x, 0)
            self.assertNotEqual(v1, v2)

    def testBracketOperator(self):
        for (Vec, dim) in VecTypes:
            v = Vec()
            for i in range(dim):
              v[i] = 1
              v[i] += 2
              self.assertEqual(v[i], 3)

    def testXYZW(self):
        for (Vec, dim) in VecTypes:
            v = Vec()
            if dim >= 1:
                v.x = 1
                v.x += 2
                self.assertEqual(v.x, 3)
            if dim >= 2:
                v.y = 2
                v.y += 3
                self.assertEqual(v.y, 5)
            if dim >= 3:
                v.z = 3
                v.z += 4
                self.assertEqual(v.z, 7)
            if dim >= 4:
                v.w = 4
                v.w += 5
                self.assertEqual(v.w, 9)

    def testArithmeticOperators(self):
        for Vec in Vec2Types:
            v1 = Vec(1, 2)
            v1 += Vec(3, 4)
            self.assertEqual(v1, Vec(4, 6))
            v2 = v1 + Vec(1, 1)
            self.assertEqual(v1, Vec(4, 6))
            self.assertEqual(v2, Vec(5, 7))
            v3 = +v2
            self.assertEqual(v2, Vec(5, 7))
            self.assertEqual(v3, Vec(5, 7))
            v3 = +v3
            self.assertEqual(v3, Vec(5, 7))
            v2 -= Vec(3, 2)
            self.assertEqual(v2, Vec(2, 5))
            v1 = v1 - v2
            self.assertEqual(v1, Vec(2, 1))
            v3 = - v2
            self.assertEqual(v2, Vec(2, 5))
            self.assertEqual(v3, Vec(-2, -5))
            v3 = - v3
            self.assertEqual(v3, Vec(2, 5))
            v1 *= 2
            self.assertEqual(v1, Vec(4, 2))
            v3 = 3 * v1 * 2
            self.assertEqual(v3, Vec(24, 12))
            v3 /= 2
            self.assertEqual(v3, Vec(12, 6))
            v4 = v3 / 12
            self.assertEqual(v4, Vec(1, 0.5))

        for Vec in Vec3Types:
            v1 = Vec(1, 2, 3)
            v1 += Vec(3, 4, 5)
            self.assertEqual(v1, Vec(4, 6, 8))
            v2 = v1 + Vec(1, 1, 1)
            self.assertEqual(v1, Vec(4, 6, 8))
            self.assertEqual(v2, Vec(5, 7, 9))
            v3 = +v2
            self.assertEqual(v2, Vec(5, 7, 9))
            self.assertEqual(v3, Vec(5, 7, 9))
            v3 = +v3
            self.assertEqual(v3, Vec(5, 7, 9))
            self.assertEqual(v3, Vec(5, 7, 9))
            v2 -= Vec(4, 3, 2)
            self.assertEqual(v2, Vec(1, 4, 7))
            v1 = v1 - v2
            self.assertEqual(v1, Vec(3, 2, 1))
            v3 = - v2
            self.assertEqual(v2, Vec(1, 4, 7))
            self.assertEqual(v3, Vec(-1, -4, -7))
            v3 = - v3
            self.assertEqual(v3, Vec(1, 4, 7))
            v1 *= 2
            self.assertEqual(v1, Vec(6, 4, 2))
            v3 = 3 * v1 * 2
            self.assertEqual(v3, Vec(36, 24, 12))
            v3 /= 2
            self.assertEqual(v3, Vec(18, 12, 6))
            v4 = v3 / 12
            self.assertEqual(v4, Vec(1.5, 1, 0.5))

        for Vec in Vec4Types:
            v1 = Vec(1, 2, 3, 4)
            v1 += Vec(3, 4, 5, 6)
            self.assertEqual(v1, Vec(4, 6, 8, 10))
            v2 = v1 + Vec(1, 1, 1, 1)
            self.assertEqual(v1, Vec(4, 6, 8, 10))
            self.assertEqual(v2, Vec(5, 7, 9, 11))
            v3 = +v2
            self.assertEqual(v2, Vec(5, 7, 9, 11))
            self.assertEqual(v3, Vec(5, 7, 9, 11))
            v3 = +v3
            self.assertEqual(v3, Vec(5, 7, 9, 11))
            self.assertEqual(v3, Vec(5, 7, 9, 11))
            v2 -= Vec(4, 3, 2, 8)
            self.assertEqual(v2, Vec(1, 4, 7, 3))
            v1 = v1 - v2
            self.assertEqual(v1, Vec(3, 2, 1, 7))
            v3 = - v2
            self.assertEqual(v2, Vec(1, 4, 7, 3))
            self.assertEqual(v3, Vec(-1, -4, -7, -3))
            v3 = - v3
            self.assertEqual(v3, Vec(1, 4, 7, 3))
            v1 *= 2
            self.assertEqual(v1, Vec(6, 4, 2, 14))
            v3 = 3 * v1 * 2
            self.assertEqual(v3, Vec(36, 24, 12, 84))
            v3 /= 2
            self.assertEqual(v3, Vec(18, 12, 6, 42))
            v4 = v3 / 12
            self.assertEqual(v4, Vec(1.5, 1, 0.5, 3.5))

    def testComparisonOperators(self):
        for Vec in Vec2Types:
            v1 = Vec(1, 2)
            v2 = Vec(3, 4)
            self.assertTrue(v1 == v1)
            self.assertTrue(v1 == Vec(1, 2))
            self.assertTrue(v1 != v2)
            self.assertTrue(v1 != Vec(3, 4))
            self.assertTrue(v1 < v2)
            self.assertTrue(v1 < Vec(2, 0))
            self.assertTrue(v1 < Vec(1, 3))
            self.assertTrue(v1 <= v1)
            self.assertTrue(v1 <= v2)
            self.assertTrue(v1 <= Vec(2, 0))
            self.assertTrue(v1 <= Vec(1, 3))
            self.assertTrue(v2 > v1)
            self.assertTrue(v2 > Vec(2, 100))
            self.assertTrue(v2 >= v2)
            self.assertTrue(v2 >= v1)
            self.assertTrue(v2 >= Vec(2, 100))
        for Vec in Vec3Types:
            v1 = Vec(1, 2, 3)
            v2 = Vec(4, 5, 6)
            self.assertTrue(v1 == v1)
            self.assertTrue(v1 == Vec(1, 2, 3))
            self.assertTrue(v1 != v2)
            self.assertTrue(v1 != Vec(4, 5, 6))
            self.assertTrue(v1 < v2)
            self.assertTrue(v1 < Vec(2, 0, 1))
            self.assertTrue(v1 < Vec(1, 3, 2))
            self.assertTrue(v1 < Vec(1, 2, 4))
            self.assertTrue(v1 <= v1)
            self.assertTrue(v1 <= v2)
            self.assertTrue(v1 <= Vec(2, 0, 1))
            self.assertTrue(v1 <= Vec(1, 3, 2))
            self.assertTrue(v1 <= Vec(1, 2, 4))
            self.assertTrue(v2 > v1)
            self.assertTrue(v2 > Vec(2, 100, 101))
            self.assertTrue(v2 >= v2)
            self.assertTrue(v2 >= v1)
            self.assertTrue(v2 >= Vec(2, 100, 101))
        for Vec in Vec4Types:
            v1 = Vec(1, 2, 3, 4)
            v2 = Vec(3, 4, 5, 6)
            self.assertTrue(v1 == v1)
            self.assertTrue(v1 == Vec(1, 2, 3, 4))
            self.assertTrue(v1 != v2)
            self.assertTrue(v1 != Vec(3, 4, 5, 6))
            self.assertTrue(v1 < v2)
            self.assertTrue(v1 < Vec(2, 0, 1, 2))
            self.assertTrue(v1 < Vec(1, 3, 2, 1))
            self.assertTrue(v1 < Vec(1, 2, 4, 6))
            self.assertTrue(v1 < Vec(1, 2, 3, 5))
            self.assertTrue(v1 <= v1)
            self.assertTrue(v1 <= v2)
            self.assertTrue(v1 <= Vec(2, 0, 1, 2))
            self.assertTrue(v1 <= Vec(1, 3, 2, 1))
            self.assertTrue(v1 <= Vec(1, 2, 4, 6))
            self.assertTrue(v1 <= Vec(1, 2, 3, 5))
            self.assertTrue(v2 > v1)
            self.assertTrue(v2 > Vec(2, 100, 101, 102))
            self.assertTrue(v2 >= v2)
            self.assertTrue(v2 >= v1)
            self.assertTrue(v2 >= Vec(2, 100, 101, 102))

    def testLength(self):
        for Vec in Vec2Types:
            for v in [Vec(3, 4), Vec(-3, 4), Vec(3, -4), Vec(-3, -4)]:
                self.assertEqual(v.length(), 5)
                self.assertEqual(v.squaredLength(), 25)
        for Vec in Vec3Types:
            v = Vec(2, 10, 11) # Pythagorean quadruple
            self.assertEqual(v.length(), 15)
            self.assertEqual(v.squaredLength(), 225)
        for Vec in Vec4Types:
            v = Vec(2, 4, 5, 6) # Pythagorean quintuple
            self.assertEqual(v.length(), 9)
            self.assertEqual(v.squaredLength(), 81)

    def testNormalize(self):
        for Vec in Vec2Types:
            v = Vec(3, 4)
            v.normalize()
            self.assertEqual(v.length(), 1)
            self.assertEqual(v, Vec(0.6, 0.8))
        for Vec in Vec3Types:
            v = Vec(2, 10, 11)
            v.normalize()
            self.assertAlmostEqual(v.length(), 1)
            self.assertEqual(v, Vec(2/15, 10/15, 11/15))
        for Vec in Vec4Types:
            v = Vec(2, 4, 5, 6)
            v.normalize()
            self.assertAlmostEqual(v.length(), 1)
            self.assertEqual(v, Vec(2/9, 4/9, 5/9, 6/9))

    def testNormalized(self):
        for Vec in Vec2Types:
            v1 = Vec(3, 4)
            v2 = v1.normalized()
            self.assertEqual(v2.length(), 1)
            self.assertEqual(v1, Vec(3, 4))
            self.assertEqual(v2, Vec(0.6, 0.8))
        for Vec in Vec3Types:
            v1 = Vec(2, 10, 11)
            v2 = v1.normalized()
            self.assertAlmostEqual(v2.length(), 1)
            self.assertEqual(v1, Vec(2, 10, 11))
            self.assertEqual(v2, Vec(2/15, 10/15, 11/15))
        for Vec in Vec4Types:
            v1 = Vec(2, 4, 5, 6)
            v2 = v1.normalized()
            self.assertAlmostEqual(v2.length(), 1)
            self.assertEqual(v1, Vec(2, 4, 5, 6))
            self.assertEqual(v2, Vec(2/9, 4/9, 5/9, 6/9))

    def testOrthogonalize(self):
        for Vec in Vec2Types:
            v = Vec(3, 4)
            v.orthogonalize()
            self.assertEqual(v, Vec(-4, 3))

    def testOrthogonalized(self):
        for Vec in Vec2Types:
            v1 = Vec(3, 4)
            v2 = v1.orthogonalized()
            self.assertEqual(v1, Vec(3, 4))
            self.assertEqual(v2, Vec(-4, 3))

    def testDot(self):
        for Vec in Vec2Types:
            v1 = Vec(1, 2)
            v2 = Vec(3, 4)
            self.assertEqual(v1.dot(v1), 5)
            self.assertEqual(v1.dot(v2), 11)
        for Vec in Vec3Types:
            v1 = Vec(1, 2, 3)
            v2 = Vec(4, 5, 6)
            self.assertEqual(v1.dot(v1), 14)
            self.assertEqual(v1.dot(v2), 32)
        for Vec in Vec4Types:
            v1 = Vec(1, 2, 3, 4)
            v2 = Vec(5, 6, 7, 8)
            self.assertEqual(v1.dot(v1), 30)
            self.assertEqual(v1.dot(v2), 70)

    def testDet(self):
        for Vec in Vec2Types:
            v1 = Vec(1, 2)
            v2 = Vec(3, 4)
            self.assertEqual(v1.det(v1), 0)
            self.assertEqual(v1.det(v2), -2)

    def testCross(self):
        for Vec in Vec3Types:
            v1 = Vec(1, 2, 6)
            v2 = Vec(3, 5, 4)
            self.assertEqual(v1.cross(v1), Vec(0, 0, 0))
            self.assertEqual(v1.cross(v2), Vec(-22, 14, -1))

    def testAngle(self):
        places = 6
        for Vec in Vec2Types:
            # Note: the Vec2 version of angle() is the only version that may
            # output negative angles. In 3D or 4D, angles are always unsigned.
            v1 = Vec(1, 0)
            v2 = Vec(1, 1)
            v3 = Vec(-3, 0)
            v4 = Vec(-2, -2)
            self.assertEqual(v1.angle(v1), 0)
            self.assertAlmostEqual(v1.angle(v2), pi/4, places=places)
            self.assertAlmostEqual(v1.angle(v3), pi, places=places)
            self.assertAlmostEqual(v1.angle(v4), -3*pi/4, places=places)
            self.assertAlmostEqual(v2.angle(v4), pi, places=places)
            self.assertAlmostEqual(v4.angle(v3), -pi/4, places=places)
            self.assertEqual(v1.angle(), 0)
            self.assertAlmostEqual(v2.angle(), pi/4, places=places)
            self.assertAlmostEqual(v3.angle(), pi, places=places)
            self.assertAlmostEqual(v4.angle(), -3*pi/4, places=places)
        for Vec in Vec3Types:
            v1 = Vec(1, 0, 0)
            v2 = Vec(1, 1, 0)
            v3 = Vec(-3, 0, 0)
            v4 = Vec(-2, 0, -2)
            self.assertEqual(v1.angle(v1), 0)
            self.assertAlmostEqual(v1.angle(v2), pi/4, places=places)
            self.assertAlmostEqual(v1.angle(v3), pi, places=places)
            self.assertAlmostEqual(v1.angle(v4), 3*pi/4, places=places)
            self.assertAlmostEqual(v2.angle(v4), 2*pi/3, places=places)
            self.assertAlmostEqual(v4.angle(v3), pi/4, places=places)
        for Vec in Vec4Types:
            v1 = Vec(1, 0, 0, 0)
            v2 = Vec(1, 1, 0, 0)
            v3 = Vec(-3, 0, 0, 0)
            v4 = Vec(-2, 0, -2, 0)
            v5 = Vec(-2, 0, 0, -2)
            self.assertEqual(v1.angle(v1), 0)
            self.assertAlmostEqual(v1.angle(v2), pi/4, places=places)
            self.assertAlmostEqual(v1.angle(v3), pi, places=places)
            self.assertAlmostEqual(v1.angle(v4), 3*pi/4, places=places)
            self.assertAlmostEqual(v2.angle(v4), 2*pi/3, places=places)
            self.assertAlmostEqual(v4.angle(v3), pi/4, places=places)
            self.assertAlmostEqual(v1.angle(v5), 3*pi/4, places=places)
            self.assertAlmostEqual(v2.angle(v5), 2*pi/3, places=places)
            self.assertAlmostEqual(v5.angle(v3), pi/4, places=places)


    def testIsClose(self):
        for Vec in Vec2Types:
            self.assertTrue(Vec(0.0, 0.0).isClose(Vec(0.0, 0.0)))
            self.assertTrue(Vec(0.0, 42.0).isClose(Vec(0.0, 42.0)))
            self.assertTrue(Vec(42.0, 0.0).isClose(Vec(42.0, 0.0)))
            self.assertTrue(Vec(42.0, 42.0).isClose(Vec(42.0, 42.0)))
            self.assertTrue(Vec(1.03, 2.5).isClose(Vec(1.03, 2.5)))
            self.assertTrue(Vec(1.03, 2.5).isClose(Vec(1.03, 2.5000000001)))
            self.assertTrue(Vec(1.03, 2.5).isClose(Vec(1.0300000000001, 2.5)))
            self.assertTrue(Vec(1.03, 2.5).isClose(Vec(1.0300000000001, 2.5000000001)))
            self.assertFalse(Vec(1.03, 2.5).isClose(Vec(1.04, 2.5)))
            self.assertFalse(Vec(1.03, 2.5).isClose(Vec(1.03, 2.6)))
            self.assertFalse(Vec(1.03, 2.5).isClose(Vec(1.04, 2.6)))
            self.assertTrue(Vec(1.03, 2.5).isClose(Vec(1.04, 2.6), 0.1))
            self.assertTrue(Vec(1.0, 0.0).isClose(Vec(1.0, 1e-10)))
            self.assertTrue(Vec( inf,  inf).isClose(Vec( inf,  inf)))
            self.assertTrue(Vec(-inf,  inf).isClose(Vec(-inf,  inf)))
            self.assertTrue(Vec( inf, -inf).isClose(Vec( inf, -inf)))
            self.assertTrue(Vec(-inf, -inf).isClose(Vec(-inf, -inf)))
            self.assertFalse(Vec( inf,  inf).isClose(Vec(-inf,  inf)))
            self.assertFalse(Vec( inf,  inf).isClose(Vec( inf, -inf)))
            self.assertFalse(Vec( inf,  inf).isClose(Vec(-inf, -inf)))
            self.assertFalse(Vec(-inf,  inf).isClose(Vec( inf,  inf)))
            self.assertFalse(Vec(-inf,  inf).isClose(Vec( inf, -inf)))
            self.assertFalse(Vec(-inf,  inf).isClose(Vec(-inf, -inf)))
            self.assertFalse(Vec( inf, -inf).isClose(Vec( inf,  inf)))
            self.assertFalse(Vec( inf, -inf).isClose(Vec(-inf,  inf)))
            self.assertFalse(Vec( inf, -inf).isClose(Vec(-inf, -inf)))
            self.assertFalse(Vec(-inf, -inf).isClose(Vec( inf,  inf)))
            self.assertFalse(Vec(-inf, -inf).isClose(Vec(-inf,  inf)))
            self.assertFalse(Vec(-inf, -inf).isClose(Vec( inf, -inf)))
            self.assertTrue(Vec(inf, 42.0).isClose(Vec(inf, 42.0)))
            self.assertTrue(Vec(inf, 42.0).isClose(Vec(inf, 42.0000000001)))
            self.assertTrue(Vec(inf, 42.0).isClose(Vec(inf, 43.0))) # (!)
            self.assertFalse(Vec(inf, 42.0).isClose(Vec(inf, inf)))
        for Vec in Vec3Types:
            self.assertTrue(Vec(1, 2, 3).isClose(Vec(1, 2, 3)))
            self.assertTrue(Vec(1, 2, 3).isClose(Vec(1, 2, 3.0000000001)))
            self.assertTrue(Vec(1, 2, 0).isClose(Vec(1, 2, 0.0000000001)))
            self.assertFalse(Vec(1, 2, 3).isClose(Vec(1, 2, 4)))
        for Vec in Vec4Types:
            self.assertTrue(Vec(1, 2, 3, 4).isClose(Vec(1, 2, 3, 4)))
            self.assertTrue(Vec(1, 2, 3, 4).isClose(Vec(1, 2, 3, 4.0000000001)))
            self.assertTrue(Vec(1, 2, 3, 0).isClose(Vec(1, 2, 3, 0.0000000001)))
            self.assertFalse(Vec(1, 2, 3, 4).isClose(Vec(1, 2, 3, 5)))

    def testAllClose(self):
        for Vec in Vec2Types:
            self.assertTrue(Vec(0.0, 0.0).allClose(Vec(0.0, 0.0)))
            self.assertTrue(Vec(0.0, 42.0).allClose(Vec(0.0, 42.0)))
            self.assertTrue(Vec(42.0, 0.0).allClose(Vec(42.0, 0.0)))
            self.assertTrue(Vec(42.0, 42.0).allClose(Vec(42.0, 42.0)))
            self.assertTrue(Vec(1.03, 2.5).allClose(Vec(1.03, 2.5)))
            self.assertTrue(Vec(1.03, 2.5).allClose(Vec(1.03, 2.5000000001)))
            self.assertTrue(Vec(1.03, 2.5).allClose(Vec(1.0300000000001, 2.5)))
            self.assertTrue(Vec(1.03, 2.5).allClose(Vec(1.0300000000001, 2.5000000001)))
            self.assertFalse(Vec(1.03, 2.5).allClose(Vec(1.04, 2.5)))
            self.assertFalse(Vec(1.03, 2.5).allClose(Vec(1.03, 2.6)))
            self.assertFalse(Vec(1.03, 2.5).allClose(Vec(1.04, 2.6)))
            self.assertTrue(Vec(1.03, 2.5).allClose(Vec(1.04, 2.6), 0.1))
            self.assertFalse(Vec(1.0, 0.0).allClose(Vec(1.0, 1e-10)))
            self.assertFalse(Vec(1.0, 0.0).allClose(Vec(1.0, 1e-10), 0.1))
            self.assertFalse(Vec(1.0, 0.0).allClose(Vec(1.0, 1e-10), 0.1, 1e-11))
            self.assertTrue(Vec(1.0, 0.0).allClose(Vec(1.0, 1e-10), 0.1, 1e-9))
            self.assertTrue(Vec( inf,  inf).allClose(Vec( inf,  inf)))
            self.assertTrue(Vec(-inf,  inf).allClose(Vec(-inf,  inf)))
            self.assertTrue(Vec( inf, -inf).allClose(Vec( inf, -inf)))
            self.assertTrue(Vec(-inf, -inf).allClose(Vec(-inf, -inf)))
            self.assertFalse(Vec( inf,  inf).allClose(Vec(-inf,  inf)))
            self.assertFalse(Vec( inf,  inf).allClose(Vec( inf, -inf)))
            self.assertFalse(Vec( inf,  inf).allClose(Vec(-inf, -inf)))
            self.assertFalse(Vec(-inf,  inf).allClose(Vec( inf,  inf)))
            self.assertFalse(Vec(-inf,  inf).allClose(Vec( inf, -inf)))
            self.assertFalse(Vec(-inf,  inf).allClose(Vec(-inf, -inf)))
            self.assertFalse(Vec( inf, -inf).allClose(Vec( inf,  inf)))
            self.assertFalse(Vec( inf, -inf).allClose(Vec(-inf,  inf)))
            self.assertFalse(Vec( inf, -inf).allClose(Vec(-inf, -inf)))
            self.assertFalse(Vec(-inf, -inf).allClose(Vec( inf,  inf)))
            self.assertFalse(Vec(-inf, -inf).allClose(Vec(-inf,  inf)))
            self.assertFalse(Vec(-inf, -inf).allClose(Vec( inf, -inf)))
            self.assertTrue(Vec(inf, 42.0).allClose(Vec(inf, 42.0)))
            self.assertTrue(Vec(inf, 42.0).allClose(Vec(inf, 42.0000000001)))
            self.assertFalse(Vec(inf, 42.0).allClose(Vec(inf, 43.0))) # (!)
            self.assertFalse(Vec(inf, 42.0).allClose(Vec(inf, inf)))
        for Vec in Vec3Types:
            self.assertTrue(Vec(1, 2, 3).allClose(Vec(1, 2, 3)))
            self.assertTrue(Vec(1, 2, 3).allClose(Vec(1, 2, 3.0000000001)))
            self.assertFalse(Vec(1, 2, 0).allClose(Vec(1, 2, 0.0000000001)))
            self.assertFalse(Vec(1, 2, 3).allClose(Vec(1, 2, 4)))
        for Vec in Vec4Types:
            self.assertTrue(Vec(1, 2, 3, 4).allClose(Vec(1, 2, 3, 4)))
            self.assertTrue(Vec(1, 2, 3, 4).allClose(Vec(1, 2, 3, 4.0000000001)))
            self.assertFalse(Vec(1, 2, 3, 0).allClose(Vec(1, 2, 3, 0.0000000001)))
            self.assertFalse(Vec(1, 2, 3, 4).allClose(Vec(1, 2, 3, 5)))

    def testIsNear(self):
        absTol = 0.001
        for Vec in Vec2Types:
            self.assertTrue(Vec(0.0, 0.0).isNear(Vec(0.0, 0.0), absTol))
            self.assertTrue(Vec(0.0, 42.0).isNear(Vec(0.0, 42.0), absTol))
            self.assertTrue(Vec(42.0, 0.0).isNear(Vec(42.0, 0.0), absTol))
            self.assertTrue(Vec(42.0, 42.0).isNear(Vec(42.0, 42.0), absTol))
            self.assertTrue(Vec(0.0, 0.0).isNear(Vec(0.0, 0.0), 0.0))
            self.assertTrue(Vec(0.0, 42.0).isNear(Vec(0.0, 42.0), 0.0))
            self.assertTrue(Vec(42.0, 0.0).isNear(Vec(42.0, 0.0), 0.0))
            self.assertTrue(Vec(42.0, 42.0).isNear(Vec(42.0, 42.0), 0.0))
            self.assertTrue(Vec(1.03, 2.5).isNear(Vec(1.03, 2.5), absTol))
            self.assertTrue(Vec(1.03, 2.5).isNear(Vec(1.03, 2.5000000001), absTol))
            self.assertTrue(Vec(1.03, 2.5).isNear(Vec(1.0300000000001, 2.5), absTol))
            self.assertTrue(Vec(1.03, 2.5).isNear(Vec(1.0300000000001, 2.5000000001), absTol))
            self.assertFalse(Vec(1.03, 2.5).isNear(Vec(1.04, 2.5), absTol))
            self.assertFalse(Vec(1.03, 2.5).isNear(Vec(1.03, 2.6), absTol))
            self.assertFalse(Vec(1.03, 2.5).isNear(Vec(1.04, 2.6), absTol))
            self.assertFalse(Vec(1.03, 2.5).isNear(Vec(1.04, 2.6), 0.1))
            self.assertTrue(Vec(1.03, 2.5).isNear(Vec(1.04, 2.6), 0.2))
            self.assertTrue(Vec(1.0, 0.0).isNear(Vec(1.0, 1e-10), absTol))
            self.assertTrue(Vec(1.0, 0.0).isNear(Vec(1.0, 1e-10), 0.1))
            self.assertFalse(Vec(1.0, 0.0).isNear(Vec(1.0, 1e-10), 1e-11))
            self.assertTrue(Vec(1.0, 0.0).isNear(Vec(1.0, 1e-10), 1e-9))
            self.assertTrue(Vec( inf,  inf).isNear(Vec( inf,  inf), absTol))
            self.assertTrue(Vec(-inf,  inf).isNear(Vec(-inf,  inf), absTol))
            self.assertTrue(Vec( inf, -inf).isNear(Vec( inf, -inf), absTol))
            self.assertTrue(Vec(-inf, -inf).isNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec( inf,  inf).isNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec( inf,  inf).isNear(Vec( inf, -inf), absTol))
            self.assertFalse(Vec( inf,  inf).isNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec(-inf,  inf).isNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec(-inf,  inf).isNear(Vec( inf, -inf), absTol))
            self.assertFalse(Vec(-inf,  inf).isNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec( inf, -inf).isNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec( inf, -inf).isNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec( inf, -inf).isNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec(-inf, -inf).isNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec(-inf, -inf).isNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec(-inf, -inf).isNear(Vec( inf, -inf), absTol))
            self.assertTrue(Vec(inf, 42.0).isNear(Vec(inf, 42.0), absTol))
            self.assertTrue(Vec(inf, 42.0).isNear(Vec(inf, 42.0000000001), absTol))
            self.assertFalse(Vec(inf, 42.0).isNear(Vec(inf, 43.0), absTol)) # (!)
            self.assertFalse(Vec(inf, 42.0).isNear(Vec(inf, inf), absTol))
        for Vec in Vec3Types:
            self.assertTrue(Vec(1, 2, 3).isNear(Vec(1, 2, 3), absTol))
            self.assertTrue(Vec(1, 2, 3).isNear(Vec(1, 2, 3.0000000001), absTol))
            self.assertTrue(Vec(1, 2, 0).isNear(Vec(1, 2, 0.0000000001), absTol))
            self.assertFalse(Vec(1, 2, 3).isNear(Vec(1, 2, 4), absTol))
        for Vec in Vec4Types:
            self.assertTrue(Vec(1, 2, 3, 4).isNear(Vec(1, 2, 3, 4), absTol))
            self.assertTrue(Vec(1, 2, 3, 4).isNear(Vec(1, 2, 3, 4.0000000001), absTol))
            self.assertTrue(Vec(1, 2, 3, 0).isNear(Vec(1, 2, 3, 0.0000000001), absTol))
            self.assertFalse(Vec(1, 2, 3, 4).isNear(Vec(1, 2, 3, 5), absTol))

    def testAllNear(self):
        for Vec in Vec2Types:
            absTol = 0.001
            self.assertTrue(Vec(0.0, 0.0).allNear(Vec(0.0, 0.0), absTol))
            self.assertTrue(Vec(0.0, 42.0).allNear(Vec(0.0, 42.0), absTol))
            self.assertTrue(Vec(42.0, 0.0).allNear(Vec(42.0, 0.0), absTol))
            self.assertTrue(Vec(42.0, 42.0).allNear(Vec(42.0, 42.0), absTol))
            self.assertTrue(Vec(0.0, 0.0).allNear(Vec(0.0, 0.0), 0.0))
            self.assertTrue(Vec(0.0, 42.0).allNear(Vec(0.0, 42.0), 0.0))
            self.assertTrue(Vec(42.0, 0.0).allNear(Vec(42.0, 0.0), 0.0))
            self.assertTrue(Vec(42.0, 42.0).allNear(Vec(42.0, 42.0), 0.0))
            self.assertTrue(Vec(1.03, 2.5).allNear(Vec(1.03, 2.5), absTol))
            self.assertTrue(Vec(1.03, 2.5).allNear(Vec(1.03, 2.5000000001), absTol))
            self.assertTrue(Vec(1.03, 2.5).allNear(Vec(1.0300000000001, 2.5), absTol))
            self.assertTrue(Vec(1.03, 2.5).allNear(Vec(1.0300000000001, 2.5000000001), absTol))
            self.assertFalse(Vec(1.03, 2.5).allNear(Vec(1.04, 2.5), absTol))
            self.assertFalse(Vec(1.03, 2.5).allNear(Vec(1.03, 2.6), absTol))
            self.assertFalse(Vec(1.03, 2.5).allNear(Vec(1.04, 2.6), absTol))
            self.assertFalse(Vec(1.03, 2.5).allNear(Vec(1.04, 2.6), 0.09))
            self.assertTrue(Vec(1.03, 2.5).allNear(Vec(1.04, 2.6), 0.11))
            self.assertTrue(Vec(1.0, 0.0).allNear(Vec(1.0, 1e-10), absTol))
            self.assertTrue(Vec(1.0, 0.0).allNear(Vec(1.0, 1e-10), 0.1))
            self.assertFalse(Vec(1.0, 0.0).allNear(Vec(1.0, 1e-10), 1e-11))
            self.assertTrue(Vec(1.0, 0.0).allNear(Vec(1.0, 1e-10), 1e-9))
            self.assertTrue(Vec( inf,  inf).allNear(Vec( inf,  inf), absTol))
            self.assertTrue(Vec(-inf,  inf).allNear(Vec(-inf,  inf), absTol))
            self.assertTrue(Vec( inf, -inf).allNear(Vec( inf, -inf), absTol))
            self.assertTrue(Vec(-inf, -inf).allNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec( inf,  inf).allNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec( inf,  inf).allNear(Vec( inf, -inf), absTol))
            self.assertFalse(Vec( inf,  inf).allNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec(-inf,  inf).allNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec(-inf,  inf).allNear(Vec( inf, -inf), absTol))
            self.assertFalse(Vec(-inf,  inf).allNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec( inf, -inf).allNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec( inf, -inf).allNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec( inf, -inf).allNear(Vec(-inf, -inf), absTol))
            self.assertFalse(Vec(-inf, -inf).allNear(Vec( inf,  inf), absTol))
            self.assertFalse(Vec(-inf, -inf).allNear(Vec(-inf,  inf), absTol))
            self.assertFalse(Vec(-inf, -inf).allNear(Vec( inf, -inf), absTol))
            self.assertTrue(Vec(inf, 42.0).allNear(Vec(inf, 42.0), absTol))
            self.assertTrue(Vec(inf, 42.0).allNear(Vec(inf, 42.0000000001), absTol))
            self.assertFalse(Vec(inf, 42.0).allNear(Vec(inf, 43.0), absTol)) # (!)
            self.assertFalse(Vec(inf, 42.0).allNear(Vec(inf, inf), absTol))
        for Vec in Vec3Types:
            self.assertTrue(Vec(1, 2, 3).allNear(Vec(1, 2, 3), absTol))
            self.assertTrue(Vec(1, 2, 3).allNear(Vec(1, 2, 3.0000000001), absTol))
            self.assertTrue(Vec(1, 2, 0).allNear(Vec(1, 2, 0.0000000001), absTol))
            self.assertFalse(Vec(1, 2, 3).allNear(Vec(1, 2, 4), absTol))
        for Vec in Vec4Types:
            self.assertTrue(Vec(1, 2, 3, 4).allNear(Vec(1, 2, 3, 4), absTol))
            self.assertTrue(Vec(1, 2, 3, 4).allNear(Vec(1, 2, 3, 4.0000000001), absTol))
            self.assertTrue(Vec(1, 2, 3, 0).allNear(Vec(1, 2, 3, 0.0000000001), absTol))
            self.assertFalse(Vec(1, 2, 3, 4).allNear(Vec(1, 2, 3, 5), absTol))

    def testToString(self):
        v = Vec2d(1, 2)
        self.assertEqual(str(v), "(1, 2)")
        self.assertEqual(repr(v), "vgc.geometry.Vec2d(1, 2)")
        for Vec in Vec2Types:
            # Setup a French locale (if available on this system) to check that even
            # when the decimal point is ',' according to the locale, numbers are
            # still printed with '.' as decimal point.
            #
            try:
                locale.setlocale(locale.LC_ALL, 'fr_FR.UTF8')
            except:
                pass
            v = Vec(1, 2.5)
            s = "(1, 2.5)"
            self.assertEqual(str(v), s)
            self.assertEqual(repr(v), "vgc.geometry." + Vec.__name__ + s)
        for Vec in Vec3Types:
            v = Vec(1, 2, 3)
            s = "(1, 2, 3)"
            self.assertEqual(str(v), s)
            self.assertEqual(repr(v), "vgc.geometry." + Vec.__name__ + s)
        for Vec in Vec4Types:
            v = Vec(1,  2,  3,  4)
            s = "(1, 2, 3, 4)"
            self.assertEqual(str(v), s)
            self.assertEqual(repr(v), "vgc.geometry." + Vec.__name__ + s)

    def testParse(self):
        for Vec in Vec2Types:
            v = Vec(1, 2.5)
            s = "  (1 ,\n2.5) " # test various formatting variants
            self.assertEqual(Vec(s), v)
        for Vec in Vec3Types:
            v = Vec(1, 2, 3)
            s = "(1, 2, 3)"
            self.assertEqual(Vec(s), v)
        for Vec in Vec4Types:
            v = Vec(1, 2, 3, 4)
            s = "(1, 2, 3, 4)"
            self.assertEqual(Vec(s), v)


class TestVecArray(unittest.TestCase):

    def testDefaultConstructor(self):
        for (VecArray, Vec, dim) in VecArrayTypes:
            a = VecArray()
            self.assertEqual(len(a), 0)

    def testInitializingConstructors(self):
        for (VecArray, Vec, dim) in VecArrayTypes:
            n = 3
            if dim == 2:
                x0 = Vec(1, 2)
            elif dim == 3:
                x0 = Vec(1, 2, 3)
            elif dim == 4:
                x0 = Vec(1, 2, 3, 4)

            a1 = VecArray(n)
            self.assertEqual(len(a1), n)
            for x in a1:
                self.assertEqual(x, Vec())

            a2 = VecArray(n, x0)
            self.assertEqual(len(a2), n)
            for x in a2:
                self.assertEqual(x, x0)

            if dim == 2:
                a3 = VecArray([(1, 2), (3, 4)])
                self.assertEqual(len(a3), 2)
                self.assertEqual(a3[0], Vec(1, 2))
                self.assertEqual(a3[1], Vec(3, 4))
            elif dim == 3:
                a3 = VecArray([(1, 2, 3), (3, 4, 5)])
                self.assertEqual(len(a3), 2)
                self.assertEqual(a3[0], Vec(1, 2, 3))
                self.assertEqual(a3[1], Vec(3, 4, 5))
            elif dim == 4:
                a3 = VecArray([(1, 2, 3, 4), (3, 4, 5, 6)])
                self.assertEqual(len(a3), 2)
                self.assertEqual(a3[0], Vec(1, 2, 3, 4))
                self.assertEqual(a3[1], Vec(3, 4, 5, 6))

    def testAppend(self):
        for (VecArray, Vec, dim) in VecArrayTypes:
            if dim == 2:
                a = VecArray()
                a.append(Vec(1, 2))
                a.append(Vec(3, 4))
                self.assertEqual(a, VecArray([(1, 2), (3, 4)]))
            elif dim == 3:
                a = VecArray()
                a.append(Vec(1, 2, 3))
                self.assertEqual(a, VecArray([(1, 2, 3)]))
            elif dim == 4:
                a = VecArray()
                a.append(Vec(1, 2, 3, 4))
                self.assertEqual(a, VecArray([(1, 2, 3, 4)]))

    def testParse(self):
        for (VecArray, Vec, dim) in VecArrayTypes:
            a = VecArray("[]")
            self.assertTrue(a == VecArray())
            if dim == 2:
                a = VecArray("  [ (  1, 2), (3, 4.5)]")
                self.assertTrue(a == VecArray([(1, 2), (3, 4.5)]))
            elif dim == 3:
                a = VecArray("[(1, 2, 3), (4, 5, 6)]")
                self.assertTrue(a == VecArray([(1, 2, 3), (4, 5, 6)]))
            elif dim == 4:
                a = VecArray("[(1, 2, 3, 4), (4, 5, 6, 7)]")
                self.assertTrue(a == VecArray([(1, 2, 3, 4), (4, 5, 6, 7)]))

    def testContains(self):
        for (VecArray, Vec, dim) in VecArrayTypes:
            if dim == 2:
                a3 = VecArray([(1, 2), (3, 4), (5, 6)])
                self.assertTrue(Vec(3, 4) in a3)
                self.assertTrue(a3.__contains__(Vec(3, 4)))
                self.assertFalse(Vec(4, 3) in a3)
                self.assertFalse(a3.__contains__(Vec(4, 3)))
                self.assertTrue(Vec(4, 3) not in a3)
            if dim == 3:
                a3 = VecArray([(1, 2, 3), (3, 4, 5)])
                self.assertTrue(Vec(3, 4, 5) in a3)
                self.assertFalse(Vec(4, 3, 5) in a3)
                self.assertTrue(Vec(4, 3, 5) not in a3)
            if dim == 4:
                a3 = VecArray([(1, 2, 3, 4), (3, 4, 5, 6)])
                self.assertTrue(Vec(3, 4, 5, 6) in a3)
                self.assertFalse(Vec(4, 3, 5, 6) in a3)
                self.assertTrue(Vec(4, 3, 5, 6) not in a3)


if __name__ == '__main__':
    unittest.main()
