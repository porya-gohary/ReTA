#ifndef STATE_HPP
#define STATE_HPP

#include <cassert>
#include <functional> // for hash
#include <algorithm>
#include "job.hpp"
#include "formattedLog.hpp"

template<class Time>
class state {
private:
    unsigned long stateID;
    // state's time stamp
    Time timeStamp;
    // set of dispatched jobs
    std::vector<job<Time>> dispatched;
    // set of finish times for dispatched jobs
    std::unordered_map<jobID, Interval<Time>> jobsFinishTimes;
    // system availability intervals
    std::vector<std::vector<Interval<Time>>> processorAvailability;
    // state's event set
    std::vector<Time> eventSet;
    // state hash
    std::size_t lookupKey;
    // Considering completion events
    const bool completionEvents = false;

public:
    // initial state -- nothing yet has dispatched, processors are all available
    state(const std::vector<unsigned int> &resourceSet, std::vector<Time> eventSet, bool completionEvents)
            : timeStamp(0), stateID(0), dispatched(),
              eventSet(eventSet), completionEvents(completionEvents), lookupKey(0x9a9a9a9a9a9a9a9aUL) {
        for (auto &resource: resourceSet) {
            processorAvailability.push_back(std::vector<Interval<Time>>());
            for (unsigned int i = 0; i < resource; i++) {
                processorAvailability.back().push_back(Interval<Time>(Time(0), Time(0)));
            }
        }
        assert(processorAvailability.size() > 0);
    }

//    // dispatch transition: new state by scheduling a job in an existing state in a specific processor
//    state(const state &from, unsigned long id, const job<Time> &s, std::size_t processor, Interval<Time> finishTime)
//            : timeStamp(from.timeStamp), dispatched(from.dispatched), jobsFinishTimes(from.jobsFinishTimes),
//              processorAvailability(from.processorAvailability), eventSet(from.eventSet) {
//        stateID = id;
//        dispatched.push_back(s);
//        jobsFinishTimes.emplace(s.getID(), finishTime);
//        processorAvailability[processor] = finishTime;
//        addEvent(finishTime.min());
//        addEvent(finishTime.max());
//        lookupKey = from.getLookupKey() ^ makeKey(s, from.timeStamp);
//    }

    // dispatch transition: new state by scheduling a job in an existing state globally
    state(const state &from, unsigned long id, const job<Time> &s, std::size_t processor, Interval<Time> finishTime)
            : timeStamp(from.timeStamp), dispatched(from.dispatched), jobsFinishTimes(from.jobsFinishTimes),
              eventSet(from.eventSet), completionEvents(from.completionEvents),
              processorAvailability(from.processorAvailability) {
        stateID = id;
        dispatched.push_back(s);
        jobsFinishTimes.emplace(s.getID(), finishTime);
        if (completionEvents) {
            addEvent(finishTime.min());
            addEvent(finishTime.max());
        }

        lookupKey = from.getLookupKey() ^ makeKey(s, from.timeStamp);

        // update processor availability
        std::vector<Time> ca, pa;

        pa.push_back(finishTime.min());
        ca.push_back(finishTime.max());

        // skip first element in from.core_avail
        for (int i = 1; i < from.processorAvailability[processor].size(); i++) {
//            pa.push_back(std::max(est, from.processorAvailability[processor][i].min()));
//            ca.push_back(std::max(est, from.processorAvailability[processor][i].max()));
            pa.push_back(from.processorAvailability[processor][i].min());
            ca.push_back(from.processorAvailability[processor][i].max());
        }

        // sort in non-decreasing order
        std::sort(pa.begin(), pa.end());
        std::sort(ca.begin(), ca.end());

        for (int i = 0; i < from.processorAvailability[processor].size(); i++) {
            processorAvailability[processor][i] = Interval<Time>(pa[i], ca[i]);
        }


    }

    // time transition: new state by advancing time in an existing state
    state(const state &from, unsigned long id, Time time)
            : timeStamp(time), dispatched(from.dispatched), jobsFinishTimes(from.jobsFinishTimes),
              processorAvailability(from.processorAvailability), eventSet(from.eventSet),
              completionEvents(from.completionEvents) {
        stateID = id;

        // new time stamp after a time transition should be greater than the previous one
        assert(timeStamp > from.timeStamp);

        // remove events that are no longer valid
        eventSet.erase(std::remove_if(eventSet.begin(), eventSet.end(), [time](Time e) { return e <= time; }),
                       eventSet.end());

        lookupKey = from.getLookupKey() ^ makeKey(time);
    }

    void addEvent(Time time) {
        if (std::find(eventSet.begin(), eventSet.end(), time) == eventSet.end()) {
            eventSet.push_back(time);
        }
        std::sort(eventSet.begin(), eventSet.end());
    }

    Time getNextEventTime() const {
        return eventSet[0];
    }

    unsigned long getNumberOfDispatchedJobs() const {
        return dispatched.size();
    }

    // make hash key for dispatch transition
    std::size_t makeKey(const job<Time> &s, Time time) const {
        auto h = std::hash<Time>{};
        std::size_t key = h(s.getID().task);
        key = (key << 4) ^ h(s.getID().job);
        key = (key << 4) ^ h(s.getArrival().until());
        key = (key << 4) ^ h(s.getCost().until());
        key = (key << 4) ^ h(s.getDeadline());
//        key = (key << 4) ^ h(time);

        return key;

    }

    // make hash key for time transition
    std::size_t makeKey(Time time) const {
        auto h = std::hash<Time>{};
        std::size_t key = h(time);

        return key;

    }

    // get processor availability
    Interval<Time> getProcessorAvailability(std::size_t p_id, std::size_t n) const {
        return processorAvailability[p_id][n - 1];
    }

    std::size_t getLookupKey() const {
        return lookupKey;
    }

    unsigned long getStateID() const {
        return stateID;
    }

    bool isDispatched(const job<Time> s) const {
        if (std::find(dispatched.begin(), dispatched.end(), s) != dispatched.end()) {
            return true;
        } else {
            return false;
        }
    }

    bool operator==(const state &other) const {
        return this->lookupKey == other.lookupKey;
    }

    bool sameJobDispatched(const state &other) const {
        if (this->dispatched.size() != other.dispatched.size()) {
            return false;
        }

        for (auto s: dispatched) {
            if (!other.isDispatched(s)) {
                return false;
            }
        }
        return true;
    }

    bool sameTimeStamp(const state &other) const {
        return this->timeStamp == other.timeStamp;
    }

    bool canMergeWith(const state &other) const {
        assert(processorAvailability.size() == other.processorAvailability.size());

//        if (this->getLookupKey() != other.getLookupKey()) {
//            return false;
//        }

        if (!sameTimeStamp(other)) {
            return false;
        }

        if (!sameJobDispatched(other)) {
            return false;
        }

        return true;
    }

    bool tryToMerge(const state &other) {
        if (!canMergeWith(other)) {
            return false;
        }
        // vector to collect joint certain jobs
        std::unordered_map<jobID, Interval<Time>> jointJobs;

        auto it1 = jobsFinishTimes.begin();

        while (it1 != jobsFinishTimes.end()) {
            jointJobs.emplace(it1->first, it1->second | other.jobsFinishTimes.find(it1->first)->second);
            it1++;
        }

        // replace the old intervals with the new ones
        jobsFinishTimes = jointJobs;

        // merge availability intervals
        for (std::size_t i = 0; i < processorAvailability.size(); i++) {
            for (std::size_t j = 0; j < processorAvailability[i].size(); j++) {
                processorAvailability[i][j] |= other.processorAvailability[i][j];
            }
        }

        // merge event set (union of two sets)
        for (auto e: other.eventSet) {
            auto it = std::find(eventSet.begin(), eventSet.end(), e);
            if (it == eventSet.end()) {
                addEvent(e);
            }
        }


        return true;
    }

    // get the state's time stamp
    Time getTimeStamp() const {
        return timeStamp;
    }

    // get the state's event set
    std::vector<Time> getEventSet() const {
        return eventSet;
    }

    // get the state's dispatched jobs
    std::vector<job<Time>> getDispatched() const {
        return dispatched;
    }

    // get the state's jobs finish times
    std::unordered_map<jobID, Interval<Time>> getJobsFinishTimes() const {
        return jobsFinishTimes;
    }

    // get the state's hash
    std::size_t getKey() const {
        return lookupKey;
    }

    friend std::ostream &operator<<(std::ostream &os, const state &s) {
        os << "State " << s.stateID << ": {";
        os << "TS: " << s.timeStamp << ", ";
        os << "S^D: [";
        for (auto &seg: s.dispatched) {
            os << seg << ", ";
        }
        os << "], ";
        os << "S^F: [";
        for (auto &seg: s.jobsFinishTimes) {
            os << "(" << seg.first << ", " << seg.second << "), ";
        }
        os << "], ";
        os << "A: [";
        for (auto &seg: s.processorAvailability) {
            os << "{";
            for (auto &intv: seg) {
                os << intv << ", ";
            }
            os << "}, ";
        }
        os << "], ";
        os << "e: [";
        for (auto &seg: s.eventSet) {
            os << seg << ", ";
        }
        os << "], ";
        return os;
    }

    std::string getStateLabel() const {
        std::stringstream os;
        os << "State " << stateID << ": \\n";
        os << "TS: " << timeStamp << ", \\n";
        os << "S^D: [";
        for (auto &seg: dispatched) {
            os << seg.getID().string() << ", ";
        }
        os << "], \\n";
        os << "A: [";
        for (auto &av: processorAvailability) {
            os << "{";
            for (auto &intv: av) {
                os << intv << ", ";
            }
            os << "}, ";
        }
        os << "], \\n";
//        os << "e: [";
//        for (auto &ev: eventSet) {
//            os << ev << ", ";
//        }
//        os << "] ";
        return os.str();
    }
};

namespace std {
    template<class T>
    struct hash<state < T>> {
    std::size_t operator()(state < T >
    const& t) const {
    return t.

    getHash();
}

};
}

#endif //STATE_HPP
