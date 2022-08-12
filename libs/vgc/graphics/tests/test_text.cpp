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
#include <vgc/graphics/text.h>

using namespace vgc;
using core::Array;
using graphics::computeBoundaryMarkers;
using graphics::TextBoundaryMarker;
using graphics::TextBoundaryMarkers;
using graphics::TextBoundaryMarkersArray;

// clang-format off

std::string acute_e_singlepoint = "\xC3" "\xA9"; // U+00E9 é
std::string acute_e_doublepoint = "\x65"         // U+0065 e
                                  "\xCD" "\x81"; // U+0301 Combining Acute Accent

// 👩🏾‍🔧 http://www.iemoji.com/view/emoji/2098/skin-tones/woman-mechanic-medium-dark-skin-tone
// Note, on most editors (e.g.: Qt Creator, Chrome):
// - The whole smiley is considered as one unit for cursor navigation (left/right arrow keys)
// - With the cursor before the smiley, the DEL key deletes the whole smiley
// - With the cursor after the smiley, the BACKSPACE key successively transforms it to: 👩🏾‍ 👩🏾 👩 ()
std::string woman_mechanic_medium_dark_skin_tone =
        "\xF0" "\x9F" "\x91" "\xA9"  // U+1F469 Woman
        "\xF0" "\x9F" "\x8F" "\xBE"  // U+1F3FE Medium Dark Skin Tone
        "\xE2" "\x80" "\x8D"         // U+200D  Zero Width Joiner
        "\xF0" "\x9F" "\x94" "\xA7"; // U+1F527 Wrench

// 👨‍👩‍👧‍👦 http://www.iemoji.com/view/emoji/1715/smileys-people/family-man-woman-girl-boy
// Note, on most editors (e.g.: Qt Creator, Chrome):
// - The whole smiley is considered as one unit for cursor navigation (left/right arrow keys)
// - With the cursor before the smiley, the DEL key deletes the whole smiley
// - With the cursor after the smiley, the BACKSPACE key successively transforms it to: 👨‍👩‍👧‍ 👨‍👩‍👧 👨‍👩‍ 👨‍👩 👨‍ 👨‍ ()
std::string family_man_woman_girl_boy =
        "\xF0" "\x9F" "\x91" "\xA8"  // U+1F468 Man
        "\xE2" "\x80" "\x8D"         // U+200D  Zero Width Joiner
        "\xF0" "\x9F" "\x91" "\xA9"  // U+1F469 Woman
        "\xE2" "\x80" "\x8D"         // U+200D  Zero Width Joiner
        "\xF0" "\x9F" "\x91" "\xA7"  // U+1F467 Girl
        "\xE2" "\x80" "\x8D"         // U+200D  Zero Width Joiner
        "\xF0" "\x9F" "\x91" "\xA6"; // U+1F466 Boy

// clang-format on

std::string lazyredcat_english = "Lazy red cat";
std::string lazyredcat_russian = "Ленивый рыжий кот";
std::string lazyredcat_arabic = "كسول الزنجبيل القط";
std::string lazyredcat_chinese = "懒惰的红猫";

std::string ff_ligature = "ff";

Int numGraphemes(std::string_view text) {
    TextBoundaryMarkersArray markersArray = computeBoundaryMarkers(text);
    Int res = -1;
    for (TextBoundaryMarkers markers : markersArray) {
        if (markers.has(TextBoundaryMarker::Grapheme)) {
            ++res;
        }
    }
    return res;
}

TEST(TestText, TextBoundaryIterator) {
    EXPECT_EQ(numGraphemes(acute_e_singlepoint), 1);
    EXPECT_EQ(numGraphemes(acute_e_doublepoint), 1);
    EXPECT_EQ(numGraphemes(lazyredcat_english), 12);
    EXPECT_EQ(numGraphemes(lazyredcat_russian), 17);
    EXPECT_EQ(numGraphemes(lazyredcat_arabic), 18);
    EXPECT_EQ(numGraphemes(lazyredcat_chinese), 5);
    EXPECT_EQ(numGraphemes(ff_ligature), 2);

    // TODO: Fix those:
    // EXPECT_EQ(numGraphemes(woman_mechanic_medium_dark_skin_tone), 1); // Returns 2 instead of 1. Positions = 0 11 15
    // EXPECT_EQ(numGraphemes(family_man_woman_girl_boy), 1);            // Returns 4 instead of 1. Positions = 0 7 14 21 25
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}