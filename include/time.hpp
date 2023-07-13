#ifndef TIME_HPP
#define TIME_HPP

#include <utility>
#include <limits>

#include "interval.hpp"

// discrete time
using discreteTime = long long;

// dense time
using denseTime = double;



inline Interval<discreteTime> I(const discreteTime &a, const discreteTime &b) {
    return Interval<discreteTime>{a, b};
}

// inline Interval<denseTime> I(const denseTime &a, const denseTime &b)
// {
// 	return Interval<denseTime>{a, b};
// }

namespace timeModel {

    template<typename T>
    struct constants
    {
        static constexpr T infinity()
        {
            return std::numeric_limits<T>::max();
        }

        // minimal time distance before some event
        static constexpr T epsilon()
        {
            return T(1);
        }

        // a deadline miss of a magnitude of less than the given tolerance is
        // ignored as noise
        static constexpr T deadlineMissTolerance()
        {
            return T(0);
        }
    };


    template<>
    struct constants<denseTime>
    {
        static constexpr denseTime infinity()
        {
            return std::numeric_limits<denseTime>::infinity();
        }

        static constexpr denseTime epsilon()
        {
            return std::numeric_limits<denseTime>::epsilon();
        }

        static constexpr denseTime deadlineMissTolerance()
        {
            // assuming we work with microseconds, this is one picosecond
            // (i.e., much less than one processor cycle)
            return 1E-6;
        }

    };

}


#endif

