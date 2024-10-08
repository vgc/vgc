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

# First two are the input segments (a1, b1) and (a2, b2)
# Then either:
# - nothing (if no expected intersection)
# - the expected intersection point and t1, t2 parameters
# - the expected intersection segment and s1, t1, s2, t2 parameters
#
# All the segments are given x-ordered or vertical y-ordered (see implementation).
# Note that for most of these, we are using values which are exactly representable
# as floats.
#
intersectData = [
    # X-Ordered non-interesecting (fast return: x-range doesn't intersect)
    [(1, 1), (3, 5), (4, 2), (6, 3)],

    # X-Ordered non-intersecting (s1Cross > 0 || s2Cross > 0)
    # Note: the subcase a2Sign == 0 may be impossible as it would fast-return
    #       due to a2x > b1x. but perhaps numerical errors might make it possible.
    #       No proof of impossibility or counter-example found yet.
    [(1, 1), (3, 5), (2, 2), (4, 1)],  # s1Cross < 0, s2Cross > 0 (perpendicular)
    [(1, 1), (3, 5), (2, 4), (3, 6)],  # s1Cross > 0, s2Cross > 0 (parallel)
    [(1, 1), (3, 5), (2, 6), (4, 6)],  # s1Cross > 0, s2Cross < 0
    [(1, 1), (3, 5), (2, 6), (4, 7)],  # s1Cross > 0, s2Cross == 0 (b2Sign == 0)
    [(1, 1), (5, 9), (4, 5), (6, 11)], # s1Cross > 0, s2Cross == 0 (b2Sign == 0)
    [(1, 1), (3, 5), (2, 2), (4, 4)],  # s1Cross == 0, s2Cross > 0 (a1Sign == 0)
    [(1, 1), (4, 6), (2, 2), (3, 4)],  # s1Cross == 0, s2Cross > 0 (b1Sign == 0)

    # X-Ordered intersecting at interior point (s1Cross < 0 && s2Cross < 0)
    [(1, 1), (5, 9), (2, 6), (10, 10), (4, 7), 0.75, 0.25],

    # X-Ordered, numCollinears == 1, a2Sign == 0
    [(1, 1), (5, 9), (4, 7), (6, 8), (4, 7), 0.75, 0],
    [(1, 1), (5, 9), (2, 3), (3, 7), (2, 3), 0.25, 0],

    # X-Ordered, numCollinears == 1, b2Sign == 0
    [(1, 1), (5, 9), (3, 4), (4, 7), (4, 7), 0.75, 1],

    # X-Ordered, numCollinears == 1, a1Sign == 0
    # Haven't yet found an example of that, or proved that it's impossible

    # X-Ordered, numCollinears == 1, b1Sign == 0
    [(1, 1), (3, 3), (2, 1), (6, 9), (3, 3), 1, 0.25],

    # X-Ordered, numCollinears == 2, a1Sign == b1Sign == 0
    # Haven't yet found an example of that, or proved that it's impossible

    # X-Ordered, numCollinears == 2, a2Sign == b2Sign == 0
    # Haven't yet found an example of that, or proved that it's impossible

    # X-Ordered, numCollinears == 2, a1Sign == a2Sign == 0, a1 == a2
    # Haven't yet found an example without a1 == a2, or proved that it's impossible
    [(1, 1), (3, 4), (1, 1), (2, 3), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (2, 4), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (2, 5), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (3, 5), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (4, 5), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (4, 4), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (4, 3), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (4, 1), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (4, 0), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (2, 0), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (2, 1), (1, 1), 0, 0],
    [(1, 1), (3, 4), (1, 1), (2, 2), (1, 1), 0, 0],

    # X-Ordered, numCollinears == 2, a1Sign == b2Sign == 0
    # Haven't yet found an example of that, or proved that it's impossible

    # X-Ordered, numCollinears == 2, a2Sign == b1Sign == 0, a2 == b1
    # Haven't yet found an example without a2 == b1, or proved that it's impossible
    [(1, 1), (3, 4), (3, 4), (4, 5), (3, 4), 1, 0],

    # X-Ordered, numCollinears == 2, b1Sign == b2Sign == 0, b1 == b2
    # Haven't yet found an example without b1 == b2, or proved that it's impossible
    [(1, 1), (3, 4), (1, 2), (3, 4), (3, 4), 1, 1],

    # X-Ordered, numCollinears == 3
    # Haven't yet found an example, or proved that it's impossible

    # X-Ordered, numCollinears == 4, non-interesecting
    # Note that this is in fact fast-rejected before doing the collinear test
    [(1, 1), (5, 9), (6, 11), (7, 13)],

    # X-Ordered, numCollinears == 4, b1 == a2
    [(1, 1), (5, 9), (5, 9), (6, 11), (5, 9), 1, 0],

    # X-Ordered, numCollinears == 4, b1 < b2
    [(1, 1), (5, 9), (4, 7), (8, 15), (4, 7), (5, 9), 0.75, 1, 0, 0.25],
    [(1, 1), (2, 3), (1, 1), (5, 9), (1, 1), (2, 3), 0, 1, 0, 0.25],

    # X-Ordered, numCollinears == 4, b1 >= b2
    [(1, 1), (5, 9), (1, 1), (5, 9), (1, 1), (5, 9), 0, 1, 0, 1],
    [(1, 1), (5, 9), (1, 1), (2, 3), (1, 1), (2, 3), 0, 0.25, 0, 1],
    [(1, 1), (5, 9), (2, 3), (5, 9), (2, 3), (5, 9), 0.25, 1, 0, 1],
    [(1, 1), (5, 9), (2, 3), (4, 7), (2, 3), (4, 7), 0.25, 0.75, 0, 1],

    # X-Ordered with vertical y-ordered
    [(1, 1), (5, 9), (0, 2), (0, 8)],
    [(1, 1), (5, 9), (1, 2), (1, 8)],
    [(1, 1), (5, 9), (2, 2), (2, 8), (2, 3), 0.25, 1/6],
    [(1, 1), (5, 9), (2, 4), (2, 8)],
    [(1, 1), (5, 9), (5, 2), (5, 8)],
    [(1, 1), (5, 9), (6, 2), (6, 8)],
    [(1, 1), (5, 9), (0, 0), (0, 8)],
    [(1, 1), (5, 9), (1, 0), (1, 8), (1, 1), 0, 0.125],
    [(1, 1), (5, 9), (2, 0), (2, 8), (2, 3), 0.25, 0.375],
    [(1, 1), (5, 9), (5, 6), (5, 10), (5, 9), 1, 0.75],
    [(1, 1), (5, 9), (1, 1), (1, 3), (1, 1), 0, 0],
    [(1, 1), (5, 9), (1, 0), (1, 1), (1, 1), 0, 1],
    [(1, 1), (5, 9), (5, 9), (5, 10), (5, 9), 1, 0],
    [(1, 1), (5, 9), (5, 8), (5, 9), (5, 9), 1, 1],

    # X-Ordered with point
    [(1, 1), (5, 9), (0, 3), (0, 3)],
    [(1, 1), (5, 9), (1, 3), (1, 3)],
    [(1, 1), (5, 9), (2, 3), (2, 3), (2, 3), 0.25, 0],
    [(1, 1), (5, 9), (4, 3), (4, 3)],
    [(1, 1), (5, 9), (5, 3), (5, 3)],
    [(1, 1), (5, 9), (6, 3), (6, 3)],
    [(1, 1), (5, 9), (1, 1), (1, 1), (1, 1), 0, 0],
    [(1, 1), (5, 9), (5, 9), (5, 9), (5, 9), 1, 0],

    # Vertical non-collinear
    [(1, 1), (1, 5), (2, 3), (2, 8)], # y-ordered
    [(1, 1), (1, 5), (2, 3), (2, 3)], # y-ordered with point
    [(1, 1), (1, 1), (2, 3), (2, 3)], # two points

    # Vertical collinear, y-ordered
    [(1, 1), (1, 5), (1, 6), (1, 10)],
    [(1, 1), (1, 5), (1, 5), (1, 9), (1, 5), 1, 0],
    [(1, 1), (1, 5), (1, 4), (1, 8), (1, 4), (1, 5), 0.75, 1, 0, 0.25],
    [(1, 1), (1, 5), (1, 1), (1, 17), (1, 1), (1, 5), 0, 1, 0, 0.25],
    [(1, 1), (1, 5), (1, 1), (1, 5), (1, 1), (1, 5), 0, 1, 0, 1],
    [(1, 1), (1, 5), (1, 1), (1, 4), (1, 1), (1, 4), 0, 0.75, 0, 1],
    [(1, 1), (1, 5), (1, 2), (1, 5), (1, 2), (1, 5), 0.25, 1, 0, 1],
    [(1, 1), (1, 5), (1, 2), (1, 4), (1, 2), (1, 4), 0.25, 0.75, 0, 1],

    # Vertical collinear, y-ordered with point
    [(1, 1), (1, 5), (1, 6), (1, 6)],
    [(1, 1), (1, 5), (1, 5), (1, 5), (1, 5), 1, 0],
    [(1, 1), (1, 5), (1, 4), (1, 4), (1, 4), 0.75, 0],
    [(1, 1), (1, 5), (1, 1), (1, 1), (1, 1), 0, 0],
    [(1, 1), (1, 5), (1, 0), (1, 0)],

    # Vertical collinear, two points
    [(1, 1), (1, 1), (1, 2), (1, 2)],
    [(1, 1), (1, 1), (1, 1), (1, 1), (1, 1), 0, 0],

    ]
