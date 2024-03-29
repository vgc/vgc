// Copyright 2022 The VGC Developers
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

#include <vgc/ui/key.h>

namespace vgc::ui {

VGC_DEFINE_ENUM_BEGIN(Key)
    VGC_ENUM_ITEM(None, "None")

    // ASCII
    VGC_ENUM_ITEM(Space, "Space")
    VGC_ENUM_ITEM(Exclamation, "!")
    VGC_ENUM_ITEM(DoubleQuote, "\"")
    VGC_ENUM_ITEM(Hashtag, "#")
    VGC_ENUM_ITEM(Dollar, "$")
    VGC_ENUM_ITEM(Percent, "%")
    VGC_ENUM_ITEM(Ampersand, "&")
    VGC_ENUM_ITEM(Apostrophe, "'")
    VGC_ENUM_ITEM(LeftParenthesis, "(")
    VGC_ENUM_ITEM(RightParenthesis, ")")
    VGC_ENUM_ITEM(Asterisk, "*")
    VGC_ENUM_ITEM(Plus, "+")
    VGC_ENUM_ITEM(Comma, ",")
    VGC_ENUM_ITEM(Minus, "-")
    VGC_ENUM_ITEM(Period, ".")
    VGC_ENUM_ITEM(Slash, "/")
    VGC_ENUM_ITEM(Digit0, "0")
    VGC_ENUM_ITEM(Digit1, "1")
    VGC_ENUM_ITEM(Digit2, "2")
    VGC_ENUM_ITEM(Digit3, "3")
    VGC_ENUM_ITEM(Digit4, "4")
    VGC_ENUM_ITEM(Digit5, "5")
    VGC_ENUM_ITEM(Digit6, "6")
    VGC_ENUM_ITEM(Digit7, "7")
    VGC_ENUM_ITEM(Digit8, "8")
    VGC_ENUM_ITEM(Digit9, "9")
    VGC_ENUM_ITEM(Colon, ":")
    VGC_ENUM_ITEM(Semicolon, ";")
    VGC_ENUM_ITEM(LessThan, "<")
    VGC_ENUM_ITEM(Equal, "=")
    VGC_ENUM_ITEM(GreaterThan, ">")
    VGC_ENUM_ITEM(Question, "?")
    VGC_ENUM_ITEM(At, "@")
    VGC_ENUM_ITEM(A, "A")
    VGC_ENUM_ITEM(B, "B")
    VGC_ENUM_ITEM(C, "C")
    VGC_ENUM_ITEM(D, "D")
    VGC_ENUM_ITEM(E, "E")
    VGC_ENUM_ITEM(F, "F")
    VGC_ENUM_ITEM(G, "G")
    VGC_ENUM_ITEM(H, "H")
    VGC_ENUM_ITEM(I, "I")
    VGC_ENUM_ITEM(J, "J")
    VGC_ENUM_ITEM(K, "K")
    VGC_ENUM_ITEM(L, "L")
    VGC_ENUM_ITEM(M, "M")
    VGC_ENUM_ITEM(N, "N")
    VGC_ENUM_ITEM(O, "O")
    VGC_ENUM_ITEM(P, "P")
    VGC_ENUM_ITEM(Q, "Q")
    VGC_ENUM_ITEM(R, "R")
    VGC_ENUM_ITEM(S, "S")
    VGC_ENUM_ITEM(T, "T")
    VGC_ENUM_ITEM(U, "U")
    VGC_ENUM_ITEM(V, "V")
    VGC_ENUM_ITEM(W, "W")
    VGC_ENUM_ITEM(X, "X")
    VGC_ENUM_ITEM(Y, "Y")
    VGC_ENUM_ITEM(Z, "Z")
    VGC_ENUM_ITEM(LeftSquareBracket, "[")
    VGC_ENUM_ITEM(Backslash, "\\")
    VGC_ENUM_ITEM(RightSquareBracket, "]")
    VGC_ENUM_ITEM(Circumflex, "^")
    VGC_ENUM_ITEM(Underscore, "_")
    VGC_ENUM_ITEM(BackQuote, "`")
    VGC_ENUM_ITEM(LeftCurlyBracket, "{")
    VGC_ENUM_ITEM(VerticalLine, "|")
    VGC_ENUM_ITEM(RightCurlyBracket, "}")
    VGC_ENUM_ITEM(Tilde, "~")

    // Latin-1 Supplement
    //
    // TODO: Should some of the pretty names be in lowercase?
    //       Example, on French AZERTY keyboard: ç, ù, é, è, à
    //       Perhaps the key pretty name should depend on the locale
    //       What about Print Screen / Impr Écran ?
    //
    VGC_ENUM_ITEM(NoBreakSpace, "Nbsp")
    VGC_ENUM_ITEM(InvertedExclamation, "¡")
    VGC_ENUM_ITEM(Cent, "¢")
    VGC_ENUM_ITEM(Pound, "£")
    VGC_ENUM_ITEM(Currency, "¤")
    VGC_ENUM_ITEM(Yen, "¥")
    VGC_ENUM_ITEM(BrokenBar, "¦")
    VGC_ENUM_ITEM(Section, "§")
    VGC_ENUM_ITEM(Diaeresis, "¨")
    VGC_ENUM_ITEM(Copyright, "©")
    VGC_ENUM_ITEM(FeminineOrdinal, "ª")
    VGC_ENUM_ITEM(LeftGuillemet, "«")
    VGC_ENUM_ITEM(NotSign, "¬")
    VGC_ENUM_ITEM(SoftHyphen, "Shy")
    VGC_ENUM_ITEM(Registered, "®")
    VGC_ENUM_ITEM(Macron, "¯")
    VGC_ENUM_ITEM(Degree, "°")
    VGC_ENUM_ITEM(PlusMinus, "±")
    VGC_ENUM_ITEM(SuperscriptTwo, "²")
    VGC_ENUM_ITEM(SuperscriptThree, "³")
    VGC_ENUM_ITEM(Acute, "´")
    VGC_ENUM_ITEM(Mu, "µ")
    VGC_ENUM_ITEM(Paragraph, "¶")
    VGC_ENUM_ITEM(MiddleDot, "·")
    VGC_ENUM_ITEM(Cedilla, "¸")
    VGC_ENUM_ITEM(SuperscriptOne, "¹")
    VGC_ENUM_ITEM(MasculineOrdinal, "º")
    VGC_ENUM_ITEM(RightGuillemet, "»")
    VGC_ENUM_ITEM(OneQuarter, "¼")
    VGC_ENUM_ITEM(OneHalf, "½")
    VGC_ENUM_ITEM(ThreeQuarters, "¾")
    VGC_ENUM_ITEM(InvertedQuestion, "¿")
    VGC_ENUM_ITEM(AGrave, "À")
    VGC_ENUM_ITEM(AAcute, "Á")
    VGC_ENUM_ITEM(ACircumflex, "Â")
    VGC_ENUM_ITEM(ATilde, "Ã")
    VGC_ENUM_ITEM(ADiaeresis, "Ä")
    VGC_ENUM_ITEM(ARing, "Å")
    VGC_ENUM_ITEM(Ae, "Æ")
    VGC_ENUM_ITEM(CCedilla, "Ç")
    VGC_ENUM_ITEM(EGrave, "È")
    VGC_ENUM_ITEM(EAcute, "É")
    VGC_ENUM_ITEM(ECircumflex, "Ê")
    VGC_ENUM_ITEM(EDiaeresis, "Ë")
    VGC_ENUM_ITEM(IGrave, "Ì")
    VGC_ENUM_ITEM(IAcute, "Í")
    VGC_ENUM_ITEM(ICircumflex, "Î")
    VGC_ENUM_ITEM(IDiaeresis, "Ï")
    VGC_ENUM_ITEM(Eth, "Ð")
    VGC_ENUM_ITEM(NTilde, "Ñ")
    VGC_ENUM_ITEM(OGrave, "Ò")
    VGC_ENUM_ITEM(OAcute, "Ó")
    VGC_ENUM_ITEM(OCircumflex, "Ô")
    VGC_ENUM_ITEM(OTilde, "Õ")
    VGC_ENUM_ITEM(ODiaeresis, "Ö")
    VGC_ENUM_ITEM(Multiplication, "×")
    VGC_ENUM_ITEM(OStroke, "Ø")
    VGC_ENUM_ITEM(UGrave, "Ù")
    VGC_ENUM_ITEM(UAcute, "Ú")
    VGC_ENUM_ITEM(UCircumflex, "Û")
    VGC_ENUM_ITEM(UDiaeresis, "Ü")
    VGC_ENUM_ITEM(YAcute, "Ý")
    VGC_ENUM_ITEM(Thorn, "Þ")
    VGC_ENUM_ITEM(SSharp, "ß")
    VGC_ENUM_ITEM(Division, "÷")
    VGC_ENUM_ITEM(YDiaeresis, "ÿ")

    // Common keys
    VGC_ENUM_ITEM(Escape, "Esc")
    VGC_ENUM_ITEM(Tab, "Tab")
    VGC_ENUM_ITEM(Backtab, "Backtab")
    VGC_ENUM_ITEM(Backspace, "Backspace")
    VGC_ENUM_ITEM(Return, "Return")
    VGC_ENUM_ITEM(Enter, "Enter")
    VGC_ENUM_ITEM(Insert, "Insert")
    VGC_ENUM_ITEM(Delete, "Del")
    VGC_ENUM_ITEM(Pause, "Pause")
    VGC_ENUM_ITEM(PrintScreen, "PrtSc")
    VGC_ENUM_ITEM(SysRq, "SysRq")
    VGC_ENUM_ITEM(Clear, "Clear")
    VGC_ENUM_ITEM(Home, "Home")
    VGC_ENUM_ITEM(End, "End")
    VGC_ENUM_ITEM(Left, "Left")
    VGC_ENUM_ITEM(Up, "Up")
    VGC_ENUM_ITEM(Right, "Right")
    VGC_ENUM_ITEM(Down, "Down")
    VGC_ENUM_ITEM(PageUp, "PgUp")
    VGC_ENUM_ITEM(PageDown, "PgDown")

    // Modifier keys
    VGC_ENUM_ITEM(Shift, "Shift")
    VGC_ENUM_ITEM(Ctrl, "Ctrl")
    VGC_ENUM_ITEM(Meta, "Meta")
    VGC_ENUM_ITEM(Alt, "Alt")

    // Lock keys
    VGC_ENUM_ITEM(CapsLock, "CapsLock")
    VGC_ENUM_ITEM(NumLock, "NumLock")
    VGC_ENUM_ITEM(ScrollLock, "ScrollLock")

    // Function keys
    VGC_ENUM_ITEM(F1, "F1")
    VGC_ENUM_ITEM(F2, "F2")
    VGC_ENUM_ITEM(F3, "F3")
    VGC_ENUM_ITEM(F4, "F4")
    VGC_ENUM_ITEM(F5, "F5")
    VGC_ENUM_ITEM(F6, "F6")
    VGC_ENUM_ITEM(F7, "F7")
    VGC_ENUM_ITEM(F8, "F8")
    VGC_ENUM_ITEM(F9, "F9")
    VGC_ENUM_ITEM(F10, "F10")
    VGC_ENUM_ITEM(F11, "F11")
    VGC_ENUM_ITEM(F12, "F12")
    VGC_ENUM_ITEM(F13, "F13")
    VGC_ENUM_ITEM(F14, "F14")
    VGC_ENUM_ITEM(F15, "F15")
    VGC_ENUM_ITEM(F16, "F16")
    VGC_ENUM_ITEM(F17, "F17")
    VGC_ENUM_ITEM(F18, "F18")
    VGC_ENUM_ITEM(F19, "F19")
    VGC_ENUM_ITEM(F20, "F20")
    VGC_ENUM_ITEM(F21, "F21")
    VGC_ENUM_ITEM(F22, "F22")
    VGC_ENUM_ITEM(F23, "F23")
    VGC_ENUM_ITEM(F24, "F24")
    VGC_ENUM_ITEM(F25, "F25")
    VGC_ENUM_ITEM(F26, "F26")
    VGC_ENUM_ITEM(F27, "F27")
    VGC_ENUM_ITEM(F28, "F28")
    VGC_ENUM_ITEM(F29, "F29")
    VGC_ENUM_ITEM(F30, "F30")
    VGC_ENUM_ITEM(F31, "F31")
    VGC_ENUM_ITEM(F32, "F32")
    VGC_ENUM_ITEM(F33, "F33")
    VGC_ENUM_ITEM(F34, "F34")
    VGC_ENUM_ITEM(F35, "F35")

    // Other keys below

    VGC_ENUM_ITEM(LeftSuper, "LeftSuper")
    VGC_ENUM_ITEM(RightSuper, "RightSuper")
    VGC_ENUM_ITEM(Menu, "Menu")
    VGC_ENUM_ITEM(LeftHyper, "LeftHyper")
    VGC_ENUM_ITEM(RightHyper, "RightHyper")
    VGC_ENUM_ITEM(Help, "Help")
    VGC_ENUM_ITEM(LeftDirection, "LeftDirection")
    VGC_ENUM_ITEM(RightDirection, "RightDirection")

    VGC_ENUM_ITEM(Back, "Back")
    VGC_ENUM_ITEM(Forward, "Forward")
    VGC_ENUM_ITEM(Stop, "Stop")
    VGC_ENUM_ITEM(Refresh, "Refresh")

    VGC_ENUM_ITEM(VolumeDown, "VolumeDown")
    VGC_ENUM_ITEM(VolumeMute, "VolumeMute")
    VGC_ENUM_ITEM(VolumeUp, "VolumeUp")
    VGC_ENUM_ITEM(BassBoost, "BassBoost")
    VGC_ENUM_ITEM(BassUp, "BassUp")
    VGC_ENUM_ITEM(BassDown, "BassDown")
    VGC_ENUM_ITEM(TrebleUp, "TrebleUp")
    VGC_ENUM_ITEM(TrebleDown, "TrebleDown")

    VGC_ENUM_ITEM(MediaPlay, "MediaPlay")
    VGC_ENUM_ITEM(MediaStop, "MediaStop")
    VGC_ENUM_ITEM(MediaPrevious, "MediaPrevious")
    VGC_ENUM_ITEM(MediaNext, "MediaNext")
    VGC_ENUM_ITEM(MediaRecord, "MediaRecord")
    VGC_ENUM_ITEM(MediaPause, "MediaPause")
    VGC_ENUM_ITEM(MediaTogglePlayPause, "MediaTogglePlayPause")

    VGC_ENUM_ITEM(HomePage, "HomePage")
    VGC_ENUM_ITEM(Favorites, "Favorites")
    VGC_ENUM_ITEM(Search, "Search")
    VGC_ENUM_ITEM(Standby, "Standby")
    VGC_ENUM_ITEM(OpenUrl, "OpenUrl")

    VGC_ENUM_ITEM(LaunchMail, "LaunchMail")
    VGC_ENUM_ITEM(LaunchMedia, "LaunchMedia")
    VGC_ENUM_ITEM(Launch0, "Launch0")
    VGC_ENUM_ITEM(Launch1, "Launch1")
    VGC_ENUM_ITEM(Launch2, "Launch2")
    VGC_ENUM_ITEM(Launch3, "Launch3")
    VGC_ENUM_ITEM(Launch4, "Launch4")
    VGC_ENUM_ITEM(Launch5, "Launch5")
    VGC_ENUM_ITEM(Launch6, "Launch6")
    VGC_ENUM_ITEM(Launch7, "Launch7")
    VGC_ENUM_ITEM(Launch8, "Launch8")
    VGC_ENUM_ITEM(Launch9, "Launch9")
    VGC_ENUM_ITEM(LaunchA, "LaunchA")
    VGC_ENUM_ITEM(LaunchB, "LaunchB")
    VGC_ENUM_ITEM(LaunchC, "LaunchC")
    VGC_ENUM_ITEM(LaunchD, "LaunchD")
    VGC_ENUM_ITEM(LaunchE, "LaunchE")
    VGC_ENUM_ITEM(LaunchF, "LaunchF")
    VGC_ENUM_ITEM(MonBrightnessUp, "MonBrightnessUp")
    VGC_ENUM_ITEM(MonBrightnessDown, "MonBrightnessDown")
    VGC_ENUM_ITEM(KeyboardLightOnOff, "KeyboardLightOnOff")
    VGC_ENUM_ITEM(KeyboardBrightnessUp, "KeyboardBrightnessUp")
    VGC_ENUM_ITEM(KeyboardBrightnessDown, "KeyboardBrightnessDown")
    VGC_ENUM_ITEM(PowerOff, "PowerOff")
    VGC_ENUM_ITEM(WakeUp, "WakeUp")
    VGC_ENUM_ITEM(Eject, "Eject")
    VGC_ENUM_ITEM(ScreenSaver, "ScreenSaver")
    VGC_ENUM_ITEM(WWW, "WWW")
    VGC_ENUM_ITEM(Memo, "Memo")
    VGC_ENUM_ITEM(LightBulb, "LightBulb")
    VGC_ENUM_ITEM(Shop, "Shop")
    VGC_ENUM_ITEM(History, "History")
    VGC_ENUM_ITEM(AddFavorite, "AddFavorite")
    VGC_ENUM_ITEM(HotLinks, "HotLinks")
    VGC_ENUM_ITEM(BrightnessAdjust, "BrightnessAdjust")
    VGC_ENUM_ITEM(Finance, "Finance")
    VGC_ENUM_ITEM(Community, "Community")
    VGC_ENUM_ITEM(AudioRewind, "AudioRewind")
    VGC_ENUM_ITEM(BackForward, "BackForward")
    VGC_ENUM_ITEM(ApplicationLeft, "ApplicationLeft")
    VGC_ENUM_ITEM(ApplicationRight, "ApplicationRight")
    VGC_ENUM_ITEM(Book, "Book")
    VGC_ENUM_ITEM(CD, "CD")
    VGC_ENUM_ITEM(Calculator, "Calculator")
    VGC_ENUM_ITEM(ToDoList, "ToDoList")
    VGC_ENUM_ITEM(ClearGrab, "ClearGrab")
    VGC_ENUM_ITEM(Close, "Close")
    VGC_ENUM_ITEM(Copy, "Copy")
    VGC_ENUM_ITEM(Cut, "Cut")
    VGC_ENUM_ITEM(Display, "Display")
    VGC_ENUM_ITEM(DOS, "DOS")
    VGC_ENUM_ITEM(Documents, "Documents")
    VGC_ENUM_ITEM(Excel, "Excel")
    VGC_ENUM_ITEM(Explorer, "Explorer")
    VGC_ENUM_ITEM(Game, "Game")
    VGC_ENUM_ITEM(Go, "Go")
    VGC_ENUM_ITEM(ITouch, "ITouch")
    VGC_ENUM_ITEM(LogOff, "LogOff")
    VGC_ENUM_ITEM(Market, "Market")
    VGC_ENUM_ITEM(Meeting, "Meeting")
    VGC_ENUM_ITEM(MenuKB, "MenuKB")
    VGC_ENUM_ITEM(MenuPB, "MenuPB")
    VGC_ENUM_ITEM(MySites, "MySites")
    VGC_ENUM_ITEM(News, "News")
    VGC_ENUM_ITEM(OfficeHome, "OfficeHome")
    VGC_ENUM_ITEM(Option, "Option")
    VGC_ENUM_ITEM(Paste, "Paste")
    VGC_ENUM_ITEM(Phone, "Phone")
    VGC_ENUM_ITEM(Calendar, "Calendar")
    VGC_ENUM_ITEM(Reply, "Reply")
    VGC_ENUM_ITEM(Reload, "Reload")
    VGC_ENUM_ITEM(RotateWindows, "RotateWindows")
    VGC_ENUM_ITEM(RotationPB, "RotationPB")
    VGC_ENUM_ITEM(RotationKB, "RotationKB")
    VGC_ENUM_ITEM(Save, "Save")
    VGC_ENUM_ITEM(Send, "Send")
    VGC_ENUM_ITEM(Spell, "Spell")
    VGC_ENUM_ITEM(SplitScreen, "SplitScreen")
    VGC_ENUM_ITEM(Support, "Support")
    VGC_ENUM_ITEM(TaskPane, "TaskPane")
    VGC_ENUM_ITEM(Terminal, "Terminal")
    VGC_ENUM_ITEM(Tools, "Tools")
    VGC_ENUM_ITEM(Travel, "Travel")
    VGC_ENUM_ITEM(Video, "Video")
    VGC_ENUM_ITEM(Word, "Word")
    VGC_ENUM_ITEM(Xfer, "Xfer")
    VGC_ENUM_ITEM(ZoomIn, "ZoomIn")
    VGC_ENUM_ITEM(ZoomOut, "ZoomOut")
    VGC_ENUM_ITEM(Away, "Away")
    VGC_ENUM_ITEM(Messenger, "Messenger")
    VGC_ENUM_ITEM(WebCam, "WebCam")
    VGC_ENUM_ITEM(MailForward, "MailForward")
    VGC_ENUM_ITEM(Pictures, "Pictures")
    VGC_ENUM_ITEM(Music, "Music")
    VGC_ENUM_ITEM(Battery, "Battery")
    VGC_ENUM_ITEM(Bluetooth, "Bluetooth")
    VGC_ENUM_ITEM(WLAN, "WLAN")
    VGC_ENUM_ITEM(UWB, "UWB")
    VGC_ENUM_ITEM(AudioForward, "AudioForward")
    VGC_ENUM_ITEM(AudioRepeat, "AudioRepeat")
    VGC_ENUM_ITEM(AudioRandomPlay, "AudioRandomPlay")
    VGC_ENUM_ITEM(Subtitle, "Subtitle")
    VGC_ENUM_ITEM(AudioCycleTrack, "AudioCycleTrack")
    VGC_ENUM_ITEM(Time, "Time")
    VGC_ENUM_ITEM(Hibernate, "Hibernate")
    VGC_ENUM_ITEM(View, "View")
    VGC_ENUM_ITEM(TopMenu, "TopMenu")
    VGC_ENUM_ITEM(PowerDown, "PowerDown")
    VGC_ENUM_ITEM(Suspend, "Suspend")
    VGC_ENUM_ITEM(ContrastAdjust, "ContrastAdjust")
    VGC_ENUM_ITEM(LaunchG, "LaunchG")
    VGC_ENUM_ITEM(LaunchH, "LaunchH")
    VGC_ENUM_ITEM(TouchpadToggle, "TouchpadToggle")
    VGC_ENUM_ITEM(TouchpadOn, "TouchpadOn")
    VGC_ENUM_ITEM(TouchpadOff, "TouchpadOff")
    VGC_ENUM_ITEM(MicMute, "MicMute")
    VGC_ENUM_ITEM(Red, "Red")
    VGC_ENUM_ITEM(Green, "Green")
    VGC_ENUM_ITEM(Yellow, "Yellow")
    VGC_ENUM_ITEM(Blue, "Blue")
    VGC_ENUM_ITEM(ChannelUp, "ChannelUp")
    VGC_ENUM_ITEM(ChannelDown, "ChannelDown")
    VGC_ENUM_ITEM(Guide, "Guide")
    VGC_ENUM_ITEM(Info, "Info")
    VGC_ENUM_ITEM(Settings, "Settings")
    VGC_ENUM_ITEM(MicVolumeUp, "MicVolumeUp")
    VGC_ENUM_ITEM(MicVolumeDown, "MicVolumeDown")
    VGC_ENUM_ITEM(New, "New")
    VGC_ENUM_ITEM(Open, "Open")
    VGC_ENUM_ITEM(Find, "Find")
    VGC_ENUM_ITEM(Undo, "Undo")
    VGC_ENUM_ITEM(Redo, "Redo")

    VGC_ENUM_ITEM(AltGr, "AltGr")
    VGC_ENUM_ITEM(MultiKey, "MultiKey")
    VGC_ENUM_ITEM(CodeInput, "CodeInput")
    VGC_ENUM_ITEM(SingleCandidate, "SingleCandidate")
    VGC_ENUM_ITEM(MultipleCandidate, "MultipleCandidate")
    VGC_ENUM_ITEM(PreviousCandidate, "PreviousCandidate")
    VGC_ENUM_ITEM(ModeSwitch, "ModeSwitch")

    VGC_ENUM_ITEM(Kanji, "Kanji")
    VGC_ENUM_ITEM(Muhenkan, "Muhenkan")
    VGC_ENUM_ITEM(Henkan, "Henkan")
    VGC_ENUM_ITEM(Romaji, "Romaji")
    VGC_ENUM_ITEM(Hiragana, "Hiragana")
    VGC_ENUM_ITEM(Katakana, "Katakana")
    VGC_ENUM_ITEM(HiraganaKatakana, "HiraganaKatakana")
    VGC_ENUM_ITEM(Zenkaku, "Zenkaku")
    VGC_ENUM_ITEM(Hankaku, "Hankaku")
    VGC_ENUM_ITEM(ZenkakuHankaku, "ZenkakuHankaku")
    VGC_ENUM_ITEM(Touroku, "Touroku")
    VGC_ENUM_ITEM(Massyo, "Massyo")
    VGC_ENUM_ITEM(KanaLock, "KanaLock")
    VGC_ENUM_ITEM(KanaShift, "KanaShift")
    VGC_ENUM_ITEM(EisuShift, "EisuShift")
    VGC_ENUM_ITEM(EisuToggle, "EisuToggle")
    VGC_ENUM_ITEM(Hangul, "Hangul")
    VGC_ENUM_ITEM(HangulStart, "HangulStart")
    VGC_ENUM_ITEM(HangulEnd, "HangulEnd")
    VGC_ENUM_ITEM(HangulHanja, "HangulHanja")
    VGC_ENUM_ITEM(HangulJamo, "HangulJamo")
    VGC_ENUM_ITEM(HangulRomaja, "HangulRomaja")
    VGC_ENUM_ITEM(HangulJeonja, "HangulJeonja")
    VGC_ENUM_ITEM(HangulBanja, "HangulBanja")
    VGC_ENUM_ITEM(HangulPreHanja, "HangulPreHanja")
    VGC_ENUM_ITEM(HangulPostHanja, "HangulPostHanja")
    VGC_ENUM_ITEM(HangulSpecial, "HangulSpecial")

    VGC_ENUM_ITEM(DeadGrave, "DeadGrave")
    VGC_ENUM_ITEM(DeadAcute, "DeadAcute")
    VGC_ENUM_ITEM(DeadCircumflex, "DeadCircumflex")
    VGC_ENUM_ITEM(DeadTilde, "DeadTilde")
    VGC_ENUM_ITEM(DeadMacron, "DeadMacron")
    VGC_ENUM_ITEM(DeadBreve, "DeadBreve")
    VGC_ENUM_ITEM(DeadAboveDot, "DeadAboveDot")
    VGC_ENUM_ITEM(DeadDiaeresis, "DeadDiaeresis")
    VGC_ENUM_ITEM(DeadAboveRing, "DeadAboveRing")
    VGC_ENUM_ITEM(DeadDoubleAcute, "DeadDoubleAcute")
    VGC_ENUM_ITEM(DeadCaron, "DeadCaron")
    VGC_ENUM_ITEM(DeadCedilla, "DeadCedilla")
    VGC_ENUM_ITEM(DeadOgonek, "DeadOgonek")
    VGC_ENUM_ITEM(DeadIota, "DeadIota")
    VGC_ENUM_ITEM(DeadVoicedSound, "DeadVoicedSound")
    VGC_ENUM_ITEM(DeadSemiVoicedSound, "DeadSemiVoicedSound")
    VGC_ENUM_ITEM(DeadBelowDot, "DeadBelowDot")
    VGC_ENUM_ITEM(DeadHook, "DeadHook")
    VGC_ENUM_ITEM(DeadHorn, "DeadHorn")
    VGC_ENUM_ITEM(DeadStroke, "DeadStroke")
    VGC_ENUM_ITEM(DeadAboveComma, "DeadAboveComma")
    VGC_ENUM_ITEM(DeadAboveReversedComma, "DeadAboveReversedComma")
    VGC_ENUM_ITEM(DeadDoubleGrave, "DeadDoubleGrave")
    VGC_ENUM_ITEM(DeadBelowRing, "DeadBelowRing")
    VGC_ENUM_ITEM(DeadBelowMacron, "DeadBelowMacron")
    VGC_ENUM_ITEM(DeadBelowCircumflex, "DeadBelowCircumflex")
    VGC_ENUM_ITEM(DeadBelowTilde, "DeadBelowTilde")
    VGC_ENUM_ITEM(DeadBelowBreve, "DeadBelowBreve")
    VGC_ENUM_ITEM(DeadBelowDiaeresis, "DeadBelowDiaeresis")
    VGC_ENUM_ITEM(DeadInvertedBreve, "DeadInvertedBreve")
    VGC_ENUM_ITEM(DeadBelowComma, "DeadBelowComma")
    VGC_ENUM_ITEM(DeadCurrency, "DeadCurrency")
    VGC_ENUM_ITEM(Deada, "Deada")
    VGC_ENUM_ITEM(DeadA, "DeadA")
    VGC_ENUM_ITEM(Deade, "Deade")
    VGC_ENUM_ITEM(DeadE, "DeadE")
    VGC_ENUM_ITEM(Deadi, "Deadi")
    VGC_ENUM_ITEM(DeadI, "DeadI")
    VGC_ENUM_ITEM(Deado, "Deado")
    VGC_ENUM_ITEM(DeadO, "DeadO")
    VGC_ENUM_ITEM(Deadu, "Deadu")
    VGC_ENUM_ITEM(DeadU, "DeadU")
    VGC_ENUM_ITEM(DeadSmallSchwa, "DeadSmallSchwa")
    VGC_ENUM_ITEM(DeadCapitalSchwa, "DeadCapitalSchwa")
    VGC_ENUM_ITEM(DeadGreek, "DeadGreek")
    VGC_ENUM_ITEM(DeadLowLine, "DeadLowLine")
    VGC_ENUM_ITEM(DeadAboveVerticalLine, "DeadAboveVerticalLine")
    VGC_ENUM_ITEM(DeadBelowVerticalLine, "DeadBelowVerticalLine")
    VGC_ENUM_ITEM(DeadLongSolidusOverlay, "DeadLongSolidusOverlay")

    VGC_ENUM_ITEM(MediaLast, "MediaLast")
    VGC_ENUM_ITEM(Select, "Select")
    VGC_ENUM_ITEM(Yes, "Yes")
    VGC_ENUM_ITEM(No, "No")
    VGC_ENUM_ITEM(Cancel, "Cancel")
    VGC_ENUM_ITEM(Printer, "Printer")
    VGC_ENUM_ITEM(Execute, "Execute")
    VGC_ENUM_ITEM(Sleep, "Sleep")
    VGC_ENUM_ITEM(Play, "Play")
    VGC_ENUM_ITEM(Zoom, "Zoom")
    VGC_ENUM_ITEM(Exit, "Exit")
    VGC_ENUM_ITEM(Context1, "Context1")
    VGC_ENUM_ITEM(Context2, "Context2")
    VGC_ENUM_ITEM(Context3, "Context3")
    VGC_ENUM_ITEM(Context4, "Context4")
    VGC_ENUM_ITEM(Call, "Call")
    VGC_ENUM_ITEM(Hangup, "Hangup")
    VGC_ENUM_ITEM(Flip, "Flip")
    VGC_ENUM_ITEM(ToggleCallHangup, "ToggleCallHangup")
    VGC_ENUM_ITEM(VoiceDial, "VoiceDial")
    VGC_ENUM_ITEM(LastNumberRedial, "LastNumberRedial")
    VGC_ENUM_ITEM(Camera, "Camera")
    VGC_ENUM_ITEM(CameraFocus, "CameraFocus")

    VGC_ENUM_ITEM(Unknown, "Unknown")
VGC_DEFINE_ENUM_END()

} // namespace vgc::ui
