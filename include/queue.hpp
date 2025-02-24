#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include "job.hpp"

enum sortKey {
    arrival_min,
    arrival_max,
    cost_min,
    cost_max,
    deadline,
    priority,
    period,
	laxity
};

template<class Time>
struct queue {
    std::vector<jobID> elements;
    const std::unordered_map<jobID, job<Time>> &segmentsByID;

    queue(const std::unordered_map<jobID, job<Time>> &segmentsByID, std::vector<jobID> elements) :
            segmentsByID(segmentsByID), elements(elements) {}


    void sort(sortKey key, Time currentTime = 0) {
        switch (key) {
            case arrival_min:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getArrival().min() < b_segment.getArrival().min()
                           ||
                           (a_segment.getArrival().min() == b_segment.getArrival().min() && a.task < b.task)
                           ||
                           (a_segment.getArrival().min() == b_segment.getArrival().min() && a.task == b.task
                            && a.job < b.job);
                });
                break;

            case arrival_max:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getArrival().max() < b_segment.getArrival().max()
                           || (a_segment.getArrival().max() == b_segment.getArrival().max() && a.task < b.task)
                           || (a_segment.getArrival().max() == b_segment.getArrival().max() && a.task == b.task
                               && a.job < b.job);
                });
                break;

            case cost_min:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getCost().min() < b_segment.getCost().min()
                           || (a_segment.getCost().min() == b_segment.getCost().min() && a.task < b.task)
                           || (a_segment.getCost().min() == b_segment.getCost().min() && a.task == b.task
                               && a.job < b.job);
                });
                break;

            case cost_max:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getCost().max() < b_segment.getCost().max()
                           || (a_segment.getCost().max() == b_segment.getCost().max() && a.task < b.task)
                           || (a_segment.getCost().max() == b_segment.getCost().max() && a.task == b.task
                               && a.job < b.job);
                });
                break;

            case deadline:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getDeadline() < b_segment.getDeadline()
                           || (a_segment.getDeadline() == b_segment.getDeadline() && a.task < b.task)
                           || (a_segment.getDeadline() == b_segment.getDeadline() && a.task == b.task
                               && a.job < b.job);
                });
                break;

            case priority:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getPriority() < b_segment.getPriority()
                           || (a_segment.getPriority() == b_segment.getPriority() && a.task < b.task)
                           || (a_segment.getPriority() == b_segment.getPriority() && a.task == b.task
                               && a.job < b.job);
                });
                break;

            case period:
                std::sort(elements.begin(), elements.end(), [this](jobID a, jobID b) {
                    auto a_segment = segmentsByID.find(a)->second;
                    auto b_segment = segmentsByID.find(b)->second;

                    return a_segment.getTaskPeriod() < b_segment.getTaskPeriod()
                           || (a_segment.getTaskPeriod() == b_segment.getTaskPeriod() && a.task < b.task)
                           || (a_segment.getTaskPeriod() == b_segment.getTaskPeriod() && a.task == b.task
                               && a.job < b.job);
                });
                break;
			case laxity:
				std::sort(elements.begin(), elements.end(), [this, currentTime](jobID a, jobID b) {
					auto a_segment = segmentsByID.find(a)->second;
					auto b_segment = segmentsByID.find(b)->second;

					auto a_laxity = a_segment.getDeadline() - currentTime - a_segment.getCost().max();
					auto b_laxity = b_segment.getDeadline() - currentTime - b_segment.getCost().max();

					return a_laxity < b_laxity
						   || (a_laxity == b_laxity && a.task < b.task)
						   || (a_laxity == b_laxity && a.task == b.task
							   && a.job < b.job);
				});
				break;
        }
    }

    bool empty() const {
        return elements.empty();
    }

    jobID front() {
        return elements.front();
    }

    jobID back() {
        return elements.back();
    }

    job<Time> at(int i) {
        return segmentsByID.find(elements.at(i))->second;
    }

    unsigned long length() {
        return elements.size();
    }

    std::vector<jobID> getElementsByID() const {
        return elements;
    }
};



#endif
