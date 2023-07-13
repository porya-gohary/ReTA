#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include "job.hpp"
#include "queue.hpp"

template<class Time>
class scheduler {
    typedef std::unordered_map<jobID, job<Time>> segmentMap;
    segmentMap segmentsByID;

public:
    scheduler() = default;

    scheduler(segmentMap segments) : segmentsByID(segments) { }

    std::optional<jobID> callScheduler(queue<Time> &readyQueue, const std::unordered_map<std::string, unsigned int> &availableResources, Time currentTime) {
        // if the queue is empty, return an empty optional
        if (readyQueue.empty()) {
            return std::nullopt;
        }

        // user defined scheduling algorithm

        readyQueue.sort(sortKey::deadline);
        return readyQueue.front();

    }
};

#endif