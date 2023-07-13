#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <vector>
#include <algorithm>

template<class Time>
class events {
private:
    std::vector<Time> times;
    bool all = false;
    bool arrival = false;
    bool completion = false;

public:
    events() = default;

    events( bool all, bool arrival, bool completion ,std::vector<Time> times={} ) :
            times(times), all(all), arrival(arrival), completion(completion) {}

    void addEventTime(Time time) {
        times.push_back(time);
    }

    void addEventTimes(std::vector<Time> times) {
        this->times.insert(this->times.end(), times.begin(), times.end());
    }

    bool isAllEvent() const {
        return all;
    }

    bool isArrivalEvent() const {
        return arrival;
    }

    bool isCompletionEvent() const {
        return completion;
    }

    std::vector<Time> getTimes() const {
        return times;
    }
};

#endif
