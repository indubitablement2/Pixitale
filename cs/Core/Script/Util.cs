using System;
using System.Numerics;

namespace Pixitale;

public static class Util
{
    /// <summary>
    /// Returns the next power of two or n if it is already a power of two.
    /// </summary>
    /// <returns></returns>
    public static uint NextPowerOfTwo(uint n)
    {
        if (n <= 1) return 1;
        return (uint.MaxValue >> BitOperations.LeadingZeroCount(n - 1)) + 1;
    }

    /// <summary>
    /// Return the next power of two of Abs(n) with the sign of n.
    /// Eg: -3 -> -4
    /// </summary>
    public static int SignedNextPowerOfTwo(int n)
    {
        if (n == 0) return 1;
        return (int)NextPowerOfTwo((uint)Math.Abs(n)) * Math.Sign(n);
    }
}