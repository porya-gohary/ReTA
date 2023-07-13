#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <vector>
#include <memory>
#include "interval.hpp"

template<class Time>
class task {
private:
    /* data */
    std::string name;
    unsigned long taskID;
    Time jitter;
    Time period = 0;
    Interval<Time> cost;
    Time deadline = 0;
    Time priority = 0;
    unsigned int assignedProcessorSet = 0;

public:
    task() = default;

    task(std::string name, unsigned long taskID, Time jitter, Time period, Interval<Time> cost, Time deadline,
         unsigned int assignedProcessorSet = 0, Time priority = 0)
            : name(name), taskID(taskID), jitter(jitter), period(period), cost(cost), deadline(deadline),
              assignedProcessorSet(assignedProcessorSet), priority(priority) {
    }

    // Encapsulation
    void setName(std::string name) {
        this->name = name;
        this->taskID = std::stol(name.substr(name.find("T") + 1));
    }

    void setMinCost(unsigned int bcet) {
        this->cost.setMin(bcet);
    }

    void setMaxCost(unsigned int wcet) {
        this->cost.setMax(wcet);
    }

    void setPeriod(unsigned int period) {
        this->period = period;
    }

    void setDeadline(unsigned int deadline) {
        this->deadline = deadline;
    }

    unsigned int getAssignedProcessorSet() const {
        return assignedProcessorSet;
    }

    std::string getName() const {
        return name;
    }

    Time getMinCost() const {
        return cost.min();
    }

    Time getMaxCost() const {
        return cost.max();
    }

    Interval<Time> getCost() const {
        return cost;
    }

    Time getPeriod() const {
        return period;
    }

    Time getDeadline() const {
        return deadline;
    }

    unsigned long getTaskID() const {
        return taskID;
    }

    Time getJitter() const {
        return jitter;
    }

    Time getPriority() const {
        return priority;
    }

};

#endif
