#ifndef JOB_HPP
#define JOB_HPP

#include <ostream>
#include <vector>
#include <algorithm> // for find
#include <exception>

#include "time.hpp"


struct jobID {
    unsigned long job;
    unsigned long task;

    jobID(unsigned long t_id, unsigned long s_id)
            : task(t_id), job(s_id) {
    }

    jobID(const jobID &other) {
        this->job = other.job;
        this->task = other.task;
    }

    bool operator==(const jobID &other) const {
        return this->task == other.task && this->job == other.job;
    }

    bool operator<(const jobID &other) const {
        return this->task < other.task ||
               (this->task == other.task && this->job < other.job);
    }

    friend std::ostream &operator<<(std::ostream &stream, const jobID &id) {
        stream << "T" << id.task << "S" << id.job;
        return stream;
    }

    friend std::string to_string(const jobID &id) {
        std::string s = "T";
        s += std::to_string(id.task) + "S" + std::to_string(id.job);
        return s;
    }

    std::string string() const {
        std::string s = "T";
        s += std::to_string(task) + "S" + std::to_string(job);
        return s;
    }
};

template<class Time>
class job {

    typedef Time Priority; // For fixed priority scheduling policies

private:
    std::string taskName;
    std::string name;
    Interval<Time> arrival;
    Interval<Time> cost;
    Time deadline;
    Priority priority;
    Time taskPeriod;
    jobID id;
    std::size_t key;
    unsigned int assignedProcessorSet = 0;

    void computeHash() {
        auto h = std::hash<Time>{};
        key = h(arrival.from());
        key = (key << 4) ^ h(id.task);
        key = (key << 4) ^ h(id.job);
        key = (key << 4) ^ h(arrival.from());
        key = (key << 4) ^ h(arrival.until());
        key = (key << 4) ^ h(cost.from());
        key = (key << 4) ^ h(cost.upto());
        key = (key << 4) ^ h(deadline);
    }

public:
    job(unsigned long tid, unsigned long sid, Interval<Time> arrival, Interval<Time> cost, Time deadline,
        unsigned int assignedProcessorSet = 0,
        Priority priority = 0)
            : arrival(arrival), cost(cost), deadline(deadline), priority(priority), id(tid, sid), assignedProcessorSet(
            assignedProcessorSet) {
        computeHash();
    }

    job(const std::string &name, unsigned long tid, unsigned long sid, Interval<Time> arrival, Interval<Time> cost,
        Time deadline, unsigned int assignedProcessorSet = 0, Priority priority = 0, Time taskPeriod=0) : name(name), arrival(arrival),
                                                                                           cost(cost),
                                                                                           deadline(deadline),
                                                                                           priority(priority),
                                                                                           id(tid, sid),
                                                                                           assignedProcessorSet(
                                                                                                   assignedProcessorSet),
                                                                                                   taskPeriod(taskPeriod){
        // cut the string before , and assign it to task name
        taskName = name.substr(0, name.find(","));
        computeHash();
    }

    job(const std::string &name, jobID sid, Interval<Time> arrival, Interval<Time> cost,
        Time deadline, unsigned int assignedProcessorSet = 0, Priority priority = 0, Time taskPeriod=0) : name(name), arrival(arrival),
                                                                                           cost(cost),
                                                                                           deadline(deadline),
                                                                                           priority(priority), id(sid),
                                                                                           assignedProcessorSet(
                                                                                                   assignedProcessorSet),
                                                                                                   taskPeriod(taskPeriod){
        // cut the string before , and assign it to task name
        taskName = name.substr(0, name.find(","));
        computeHash();
    }
//    job(unsigned long tid, unsigned long sid, Interval<Time> arrival, Interval<Time> cost, Time deadline)
//            : arrival(arrival), cost(cost), deadline(deadline), id(tid, sid) {
//        computeHash();
//    }

    void setTaskPeriod(Time period) {
        taskPeriod = period;
    }

    Time getTaskPeriod() const {
        return taskPeriod;
    }

    Interval<Time> getArrival() const {
        return arrival;
    }

    Time getEarliestArrival() const {
        return arrival.from();
    }

    Time getLatestArrival() const {
        return arrival.until();
    }

    Interval<Time> getCost() const {
        return cost;
    }

    Time getLeastCost() const {
        return cost.from();
    }

    Time getMaximalCost() const {
        return cost.upto();
    }

    Time getDeadline() const {
        return deadline;
    }

    Priority getPriority() const {
        return priority;
    }

    jobID getID() const {
        return id;
    }

    std::size_t getHash() const {
        return key;
    }

    std::string getName() const {
        return name;
    }

    unsigned int getAssignedProcessorSet() const {
        return assignedProcessorSet;
    }

    std::string getTaskName() const {
        return taskName;
    }

    bool exceedsDeadline(Time t) const {
        return t > deadline && (t - deadline) > timeModel::constants<Time>::deadlineMissTolerance();
    }

	Interval<Time> schedulingWindow() const {
		// inclusive interval, so take off one epsilon
		return Interval<Time>{
				getEarliestArrival(),
				deadline - timeModel::constants<Time>::epsilon()};
	}

	static Interval<Time> schedulingWindow(const job &j) {
		return j.schedulingWindow();
	}

    bool operator==(const job<Time> &other) const {
        return this->id == other.id;
    }

    friend std::string to_string(const job<Time> &seg) {
        return "Segment{" + seg.id.string() + ", " + std::to_string(seg.arrival) + ", " +
               std::to_string(seg.cost) + ", " +
               std::to_string(seg.deadline) + ", " + std::to_string(seg.priority) + "}";
    }

    friend std::ostream &operator<<(std::ostream &stream, const job<Time> &seg) {
        stream << "Segment{" << seg.id << ", " << seg.arrival << ", " << seg.cost << ", " << seg.deadline << ", "
               << seg.priority << "}";
        return stream;
    }


};

namespace std {
    template<class T>
    struct hash<job < T>> {
    std::size_t operator()(job < T >
    const& j) const {
    return j.

    getHash();
}
};

template<>
struct hash<jobID> {
    std::size_t operator()(jobID const &id) const {
        hash<unsigned long> h;
        return (h(id.job) << 4) ^ h(id.task);
    }

};

}

#endif //SEGMENT_HPP
