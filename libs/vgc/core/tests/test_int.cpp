// Note: we currently haven't implemented any architecture to perform C++ tests
// (only Python tests). Therefore, the tests below are not automatically
// executed, they are just added to the repository for future use, when C++
// tests will actually be implemented.

#include <vgc/core/int.h>

using namespace vgc;
using core::NegativeIntegerError;
using core::IntegerOverflowError;

int main()
{
    // Trivial casts from type T to T.
    // TODO: also test with the minimum value and the max value.
    int_cast<bool>(bool(0));
    int_cast<char>(char(0));
    int_cast<signed char>((signed char)(0));
    int_cast<unsigned char>((unsigned char)(0));
    int_cast<short>(short(0));
    int_cast<int>(int(0));
    int_cast<long>(long(0));
    int_cast<long long>((long long)(0));
    int_cast<unsigned short>((unsigned short)(0));
    int_cast<unsigned int>((unsigned int)(0));
    int_cast<unsigned long>((unsigned long)(0));
    int_cast<unsigned long long>((unsigned long long)(0));
    int_cast<Int>(Int(0));
    int_cast<Int8>(Int8(0));
    int_cast<Int16>(Int16(0));
    int_cast<Int32>(Int32(0));
    int_cast<Int64>(Int64(0));
    int_cast<UInt>(UInt(0));
    int_cast<UInt8>(UInt8(0));
    int_cast<UInt16>(UInt16(0));
    int_cast<UInt32>(UInt32(0));
    int_cast<UInt64>(UInt64(0));

    // Casts between separate types that should work.
    // TODO: be more exhaustive than that.
    int_cast<UInt8>(Int8(127));
    int_cast<UInt8>(Int16(255));
    int_cast<UInt32>(Int32(2147483647));
    int_cast<UInt32>(Int64(4294967295));
    int_cast<Int8>(UInt8(127));
    int_cast<Int8>(Int16(127));
    int_cast<UInt8>(UInt16(255));

    // Casts between separate types that should raise an exception.
    // TODO: be more exhaustive than that.
    try { int_cast<UInt8>(Int8(-1)); abort(); } catch (NegativeIntegerError&) {}
    try { int_cast<UInt8>(Int16(256)); abort(); } catch (IntegerOverflowError&) {}
    try { int_cast<UInt32>(Int32(-1)); abort(); } catch (NegativeIntegerError&) {}
    try { int_cast<UInt32>(Int64(4294967296)); abort(); } catch (IntegerOverflowError&) {}
    try { int_cast<Int8>(UInt8(128)); abort(); } catch (IntegerOverflowError&) {}
    try { int_cast<Int8>(Int16(128)); abort(); } catch (IntegerOverflowError&) {}
    try { int_cast<UInt8>(UInt16(256)); abort(); } catch (IntegerOverflowError&) {}

    return 0;
}
