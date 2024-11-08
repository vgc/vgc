#!/usr/bin/python3

# Copyright 2024 The VGC Developers
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

import os, sys
sys.path.append(os.path.dirname(__file__))
import segment2data

from vgc.geometry import (
    Segment2d, Segment2f, Vec2d, Vec2f,
    SegmentIntersector2d, SegmentIntersector2f,
    Segment2fIntersection, Segment2dIntersection, SegmentIntersectionType)

SegmentIntersector2Types = [SegmentIntersector2d, SegmentIntersector2f]
Segment2SegmentIntersector2Segment2IntersectionTypes = [
    (Segment2d, SegmentIntersector2d, Segment2dIntersection),
    (Segment2f, SegmentIntersector2f, Segment2fIntersection)]

def swapVecXY(vec):
    if type(vec) is tuple:
        vec = vec[1], vec[0]
    else:
        vec.x, vec.y = vec.y, vec.x

# Tuples are immutable, so this cannot be done in-place
def swapTupleXY(t):
    return t[1], t[0]

def swapSegmentXY(segment):
    swapVecXY(segment.a)
    swapVecXY(segment.b)

class TestSegmentIntersector2(unittest.TestCase):
    def testConstructor(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            self.assertEqual(len(si.intersectionPoints()), 0)

    def testAddSegment(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            si.addSegment((0, 0), (1, 1))
            si.addSegment((0, 1), (1, 0))
            si.computeIntersections()
            self.assertEqual(len(si.intersectionPoints()), 1)

    def testAddOpenPolyline(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            si.addPolyline([(0, 0), (1, 1), (0, 1), (1, 0)])
            si.computeIntersections()
            self.assertEqual(len(si.intersectionPoints()), 1)

    def testAddOpenPolylineWithCommonEndpoints(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            si.addPolyline([(0, 0), (1, 1), (0, 1), (1, 0), (0, 0)])
            si.computeIntersections()
            self.assertEqual(len(si.intersectionPoints()), 2)

    def testAddClosedPolyline(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            si.addPolyline([(0, 0), (1, 1), (0, 1), (1, 0)],
                           isClosed=True)
            si.computeIntersections()
            self.assertEqual(len(si.intersectionPoints()), 1)

    def testAddClosedPolylineWithDuplicateEndpoints(self):
        for SegmentIntersector2 in SegmentIntersector2Types:
            si = SegmentIntersector2()
            si.addPolyline([(0, 0), (1, 1), (0, 1), (1, 0), (0, 0)],
                           isClosed=True, hasDuplicateEndpoints=True)
            si.computeIntersections()
            self.assertEqual(len(si.intersectionPoints()), 1)

    def testTwoSegments(self):

        intersectData = segment2data.intersectData

        for Segment2, SegmentIntersector2, Segment2Intersection in (
                Segment2SegmentIntersector2Segment2IntersectionTypes):
            for d in intersectData:
                for swapS in [False, True]:
                    for swap1 in [False, True]:
                        for swap2 in [False, True]:
                            for swapXY in [False, True]:
                                s1 = Segment2(d[0], d[1])
                                s2 = Segment2(d[2], d[3])
                                args = d[4:]
                                if swap1:
                                    s1 = Segment2(s1.b, s1.a)
                                    if s1.a != s1.b:
                                        if len(args) == 3:
                                            args[1] = 1 - args[1] # t1
                                        if len(args) == 6:
                                            args[2] = 1 - args[2] # s1
                                            args[3] = 1 - args[3] # t1
                                if swap2:
                                    s2 = Segment2(s2.b, s2.a)
                                    if s2.a != s2.b:
                                        if len(args) == 3:
                                            args[2] = 1 - args[2] # t2
                                        if len(args) == 6:
                                            args[4] = 1 - args[4] # s2
                                            args[5] = 1 - args[5] # t2
                                if swapS:
                                    s1, s2 = s2, s1
                                    if len(args) == 3:
                                        args[1], args[2] = args[2], args[1]
                                    if len(args) == 6:
                                        args[2:4], args[4:6] = args[4:6], args[2:4]
                                if swapXY:
                                    swapSegmentXY(s1)
                                    swapSegmentXY(s2)
                                    if len(args) == 3:
                                        args[0] = swapTupleXY(args[0])
                                    if len(args) == 6:
                                        args[0] = swapTupleXY(args[0])
                                        args[1] = swapTupleXY(args[1])

                                expected = Segment2Intersection(*args)

                                si = SegmentIntersector2()
                                si.addSegment(s1.a, s1.b)
                                si.addSegment(s2.a, s2.b)
                                si.computeIntersections()

                                if (expected.type == SegmentIntersectionType.Point):
                                    self.assertEqual(len(si.intersectionPoints()), 1)
                                    self.assertEqual(len(si.intersectionSubsegments()), 0)
                                    self.assertEqual(si.intersectionPoints()[0].position, expected.p)
                                    self.assertEqual(len(si.intersectionPoints()[0].segments), 2)
                                    foundS1 = False
                                    foundS2 = False
                                    for vertseg in si.intersectionPoints()[0].segments:
                                        if vertseg.segmentIndex == 0:
                                            self.assertEqual(vertseg.parameter, expected.t1)
                                            foundS1 = True
                                        if vertseg.segmentIndex == 1:
                                            self.assertEqual(vertseg.parameter, expected.t2)
                                            foundS2 = True
                                    self.assertTrue(foundS1)
                                    self.assertTrue(foundS2)

                                elif (expected.type == SegmentIntersectionType.Segment):
                                    self.assertEqual(len(si.intersectionPoints()), 2)
                                    self.assertEqual(len(si.intersectionSubsegments()), 1)
                                    self.assertEqual(si.intersectionSubsegments()[0].subsegment, expected.segment)
                                    pass
                                elif (expected.type == SegmentIntersectionType.Empty):
                                    self.assertEqual(len(si.intersectionPoints()), 0)
                                    self.assertEqual(len(si.intersectionSubsegments()), 0)

    def testThreeOverlapSegments(self):
        for Segment2, SegmentIntersector2, Segment2Intersection in (
                Segment2SegmentIntersector2Segment2IntersectionTypes):

            # Case without other segments
            si = SegmentIntersector2()
            si.addSegment((1, 0), (4, 0)) # A = o--------o
            si.addSegment((2, 0), (5, 0)) # B =    o--------o
            si.addSegment((3, 0), (6, 0)) # C =       o--------o
            si.computeIntersections()     #     x--x--x--x--x--x     < output vertices/edges
                                          #     0  1  2  3  4  5
            self.assertEqual(len(si.intersectionPoints()), 4)
            self.assertEqual(len(si.intersectionSubsegments()), 3) # (A, B), (A, C), (B, C)
            self.assertEqual(si.intersectionSubsegments()[0].subsegment, Segment2((2, 0), (3, 0)))
            self.assertEqual(si.intersectionSubsegments()[1].subsegment, Segment2((3, 0), (4, 0)))
            self.assertEqual(si.intersectionSubsegments()[2].subsegment, Segment2((4, 0), (5, 0)))


if __name__ == '__main__':
    unittest.main()
