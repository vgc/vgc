// Note: we currently haven't implemented any architecture to perform C++ tests
// (only Python tests). Therefore, the tests below are not automatically
// executed, they are just added to the repository for future use, when C++
// tests will actually be implemented.

#include <vgc/core/int.h>

using namespace vgc;
using core::RangeError;

int main()
{
    // Trivial casts from type T to T.
    // TODO: also test with the minimum value and the max value.
    int_cast<bool>(static_cast<bool>(0));
    int_cast<char>(static_cast<char>(0));
    int_cast<signed char>(static_cast<signed char>(0));
    int_cast<unsigned char>(static_cast<unsigned char>(0));
    int_cast<short>(static_cast<short>(0));
    int_cast<int>(static_cast<int>(0));
    int_cast<long>(static_cast<long>(0));
    int_cast<long long>(static_cast<long long>(0));
    int_cast<unsigned short>(static_cast<unsigned short>(0));
    int_cast<unsigned int>(static_cast<unsigned int>(0));
    int_cast<unsigned long>(static_cast<unsigned long>(0));
    int_cast<unsigned long long>(static_cast<unsigned long long>(0));
    int_cast<Int>(static_cast<Int>(0));
    int_cast<Int8>(static_cast<Int8>(0));
    int_cast<Int16>(static_cast<Int16>(0));
    int_cast<Int32>(static_cast<Int32>(0));
    int_cast<Int64>(static_cast<Int64>(0));
    int_cast<UInt>(static_cast<UInt>(0));
    int_cast<UInt8>(static_cast<UInt8>(0));
    int_cast<UInt16>(static_cast<UInt16>(0));
    int_cast<UInt32>(static_cast<UInt32>(0));
    int_cast<UInt64>(static_cast<UInt64>(0));

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
    try { int_cast<UInt8>(Int8(-1)); abort(); } catch (RangeError&) {}
    try { int_cast<UInt8>(Int16(256)); abort(); } catch (RangeError&) {}
    try { int_cast<UInt32>(Int32(-1)); abort(); } catch (RangeError&) {}
    try { int_cast<UInt32>(Int64(4294967296)); abort(); } catch (RangeError&) {}
    try { int_cast<Int8>(UInt8(128)); abort(); } catch (RangeError&) {}
    try { int_cast<Int8>(Int16(128)); abort(); } catch (RangeError&) {}
    try { int_cast<UInt8>(UInt16(256)); abort(); } catch (RangeError&) {}

    return 0;
}
