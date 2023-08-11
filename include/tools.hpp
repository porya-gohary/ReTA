#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <unordered_map>
#include "interval.hpp"
#include <fstream>
#include <cmath>
#include "job.hpp"
#include "dag.hpp"
#include "time.hpp"
#include "task.hpp"
#include "processor.hpp"

namespace tools {
    // observation window length
    long long observationWindow = 0;

    // Recursive function to return gcd of a and b
    long long gcd(long long int a, long long int b) {
        if (b == 0)
            return a;
        return gcd(b, a % b);
    }

    // Function to return LCM of two numbers
    long long lcm(long long a, long long b) {
        return (a / gcd(a, b)) * b;
    }

    //Function to calculate hyperperiod
    template<typename Time>
    Time calHyperperiod(std::vector<task<Time>> tasks) {
        Time h = 1;
        for (const auto &t_instance: tasks) {
            h = lcm(h, ceil(t_instance.getPeriod()));
        }
        return h;
    }

    //Function to generate all segments in the system
    template<typename Time>
    std::vector<job<Time>> generateSegments(std::vector<task<Time>> tasks, std::vector<job<Time>> segments) {
        std::vector<job<Time>> generatedSegments;
        observationWindow = calHyperperiod(tasks);
        for (const auto &t_instance: tasks) {
            unsigned long jobCounter = 0;
            for (long long i = 0; i < observationWindow / t_instance.getPeriod(); i++) {
                generatedSegments.emplace_back(t_instance.getName() + "," + std::to_string(jobCounter),
                                               t_instance.getTaskID(), jobCounter,
                                               Interval<Time>(t_instance.getPeriod() * i,
                                                              t_instance.getPeriod() * i + t_instance.getJitter()),
                                               t_instance.getCost(),
                                               (t_instance.getPeriod() * i) + t_instance.getDeadline(),
                                               t_instance.getAssignedProcessorSet(), t_instance.getPriority(),t_instance.getPeriod());
                jobCounter++;
            }
        }

        for (const auto &s_instance: segments) {
            generatedSegments.emplace_back(s_instance.getName(), s_instance.getID(),
                                           s_instance.getArrival(), s_instance.getCost(),
                                           s_instance.getArrival().min() + s_instance.getDeadline(),
                                           s_instance.getAssignedProcessorSet(), s_instance.getPriority());
        }

        return generatedSegments;
    }

    //Function to generate resource set
    std::vector<unsigned int> generateResourceSet(const std::vector<processor> &processors) {
        std::vector<unsigned int> resourceSet;
        for (int i = 0; i < processors.size(); i++) {
            resourceSet.push_back(processors[i].getNumCores());
        }
        return resourceSet;
    }


    template<class Time>
    struct result {
        bool schedulable;
        bool timeout;
        unsigned long numStates;
        double cpuTime;
        dag graph;
        std::unordered_map<jobID, Interval<Time>> responseTimes;
    };

    template<class Time>
    struct inputs {
        std::vector<job<Time>> segments;
    };
}

#endif //IO_HPP
