// Copyright 2021 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <vgc/core/preprocessor.h>

#define ADD(x, y) x + y

TEST(TestPreprocessor, Str) {
    const char* a = "ADD(1, 2)";
    const char* b = VGC_PP_STR(ADD(1, 2));
    EXPECT_EQ(a[0], 'A');
    EXPECT_EQ(a, "ADD(1, 2)");
    EXPECT_EQ(b, "1 + 2");
}

#define ANSWER() 42
#define CAT_ANSWER(x) VGC_PP_CAT(x, ANSWER())

TEST(TestPreprocessor, Cat) {
    constexpr int a = CAT_ANSWER(1);
    EXPECT_EQ(a, 142);
}

#define APPLY_OP(op, ...) VGC_PP_EXPAND(op(__VA_ARGS__))

TEST(TestPreprocessor, Expand) {
    constexpr int a = APPLY_OP(ADD, 1, 2);
    EXPECT_EQ(a, 3);
}

TEST(TestPreprocessor, NumArgs) {

    constexpr int a = VGC_PP_NUM_ARGS(v1);
    EXPECT_EQ(a, 1);

    constexpr int b = VGC_PP_NUM_ARGS(v1, v2, v3);
    EXPECT_EQ(b, 3);

    // clang-format off
    constexpr int c = VGC_PP_NUM_ARGS(
        v1, v2, v3, v4, v5, v6, v7, v8, v9,
        v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
        v20, v21, v22, v23, v24, v25, v26, v27, v28, v29,
        v30, v31, v32, v33, v34, v35, v36, v37, v38, v39,
        v40, v41, v42, v43, v44, v45, v46, v47, v48, v49,
        v50, v51, v52, v53, v54, v55, v56, v57, v58, v59,
        v60, v61, v62, v63, v64, v65, v66, v67, v68, v69,
        v70, v71, v72, v73, v74, v75, v76, v77, v78, v79,
        v80, v81, v82, v83, v84, v85, v86, v87, v88, v89,
        v90, v91, v92, v93, v94, v95, v96, v97, v98, v99,
        v100, v101, v102, v103, v104, v105, v106, v107, v108, v109,
        v110, v111, v112, v113, v114, v115, v116, v117, v118, v119,
        v120, v121, v122, v123, v124, v125);
    // clang-format on
    EXPECT_EQ(c, 125);
}

#define MIN_1(x) x
#define MIN_2(x, y) ((x) < (y) ? (x) : (y))
#define MIN_3(x, y, z) ((x) < (y) ? MIN_2(x, z) : MIN_2(y, z))
#define MIN(...) VGC_PP_EXPAND(VGC_PP_OVERLOAD(MIN_, __VA_ARGS__)(__VA_ARGS__))

TEST(TestPreprocessor, Overload) {
    constexpr int a = MIN(42);
    constexpr int b = MIN(42, 10);
    constexpr int c = MIN(42, 10, 25);
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 10);
    EXPECT_EQ(c, 10);
}

#define APPEND(x, t) x.append(t);

TEST(TestPreprocessor, Foreach) {
    std::string s;
    VGC_PP_FOREACH(APPEND, s, "Hello, ", "World!");
    EXPECT_EQ(s, "Hello, World!");

    std::string t;
    // clang-format off
    VGC_PP_FOREACH(APPEND, t,
               "001", "002", "003", "004", "005", "006", "007", "008", "009",
        "010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
        "020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
        "030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
        "040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
        "050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
        "060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
        "070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
        "080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
        "090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
        "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
        "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
        "120", "121", "122");
    // clang-format on
    EXPECT_EQ(t.size(), 122 * 3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
