#ifndef TRANSITION_SYSTEM_HPP
#define TRANSITION_SYSTEM_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include "state.hpp"
#include "cpuTime.hpp"
#include "dag.hpp"
#include "models/scheduler.hpp"
#include "models/events.hpp"
#include "formattedLog.hpp"
#include "csvfile.h"
#include "combinations.hpp"
#include "TextTable.hpp"
#include "queue.hpp"

#ifndef _WIN32

#include <sys/resource.h>

#else
#include "windows.h"
#include "psapi.h"
#endif


template<class Time>
class transitionSystem {
    typedef unsigned long stateID;
    typedef std::vector<job<Time>> jobSet;
	typedef const job<Time> *jobRef;
    typedef std::unordered_map<jobID, jobRef> jobRefMapByID;
    typedef std::unordered_map<jobID, job<Time>> jobMapByID;
    typedef std::vector<jobID> readyQueue;
    typedef std::vector<readyQueue> readyQueues;
	typedef IntervalLookupTable<Time, job<Time>, job<Time>::schedulingWindow> Jobs_lut;

private:
    std::unordered_map<stateID, state<Time>> statesByID;
    jobMapByID jobsByID;
	jobRefMapByID jobRefsByID;
	Jobs_lut _jobsByWin;

	const Jobs_lut &jobsByWin;


    bool beNaive = false;
    cpuTime timer;
    const jobSet jobs;
    const double timeout;
    const events<Time> systemEvents;
    const std::vector<unsigned int> resourceSet;
    const std::vector<processor> processors;
    unsigned long numDispatchedJob = 0;
    scheduler<Time> schedulingPolicy;
    std::string jobFile;

    // used in the result of the analysis
    bool aborted = false;
    bool completed = false;
    bool timedOut = false;
    unsigned long numStates = 0;
    std::unordered_map<jobID, Interval<Time>> responseTimes;
    dag transitionStructure;

public:
    // Interface for constructing the transition system naively
    static transitionSystem
    constructNaively(std::string segFile, jobSet &workload, events<Time> systemEvents,
                     std::vector<processor> &processors, double timeout) {
        std::vector<unsigned int> resourceSet = tools::generateResourceSet(processors);
        auto ts = transitionSystem(workload, systemEvents, processors, resourceSet, timeout);
        log<LOG_DEBUG>("Constructing transition system naively");
        ts.jobFile = segFile;
        ts.beNaive = true;
        ts.timer.start();
        ts.construct();
        ts.timer.stop();
        return ts;
    }

    static transitionSystem
    construct(std::string segFile, jobSet jobs, events<Time> systemEvents,
              std::vector<processor> &processors, double timeout) {
        std::vector<unsigned int> resourceSet = tools::generateResourceSet(processors);
        auto ts = transitionSystem(jobs, systemEvents, processors, resourceSet, timeout);
        log<LOG_DEBUG>("Constructing transition system");
        ts.jobFile = segFile;
        ts.beNaive = false;
        ts.timer.start();
        ts.construct();
        ts.timer.stop();
        return ts;
    }

    transitionSystem(const jobSet &workload, events<Time> systemEvents, std::vector<processor> &processors,
                     std::vector<unsigned int> &resourceSet, double timeout)
            : jobs(workload), numStates(0), timeout(timeout), systemEvents(systemEvents), processors(processors),
              resourceSet(resourceSet),
              numDispatchedJob(workload.size()), _jobsByWin(Interval<Time>{0, maxDeadline(workload)}, maxDeadline(workload) / workload.size()),
			  jobsByWin(_jobsByWin) {
        for (const job<Time> &s: jobs) {
            jobsByID.emplace(s.getID(), s);
			jobRefsByID.emplace(s.getID(), &s);
			_jobsByWin.insert(s);
        }
        schedulingPolicy = scheduler<Time>(jobsByID);

    }

    void construct() {
        makeInitialStates();

        do {

            // get ID of all leaf states
            auto leaves = transitionStructure.getLeaves();
            updateNumDispatchedJobs(leaves);

            // There should be at least one leaf state in the transition system
            assert(!leaves.empty());

			// Use an unordered_set for faster lookups
			std::unordered_set<stateID> leafSet(leaves.begin(), leaves.end());

			// remove non-leaves states (free memory)
			for (auto it = statesByID.begin(); it != statesByID.end();) {
				if (leafSet.find(it->first) == leafSet.end()) {
					it = statesByID.erase(it);
				} else {
					++it;
				}
			}

#ifndef COLLECT_TLTS_GRAPH
			// free memory
			transitionStructure.freeMemory();
#endif

            // filter unexplorable states
            auto explorableLeaves = findExplorableStates(leaves);

            // sort states by time stamp
            std::sort(explorableLeaves.begin(), explorableLeaves.end(), [this](stateID a, stateID b) {
                return statesByID.find(a)->second.getTimeStamp() <
                       statesByID.find(b)->second.getTimeStamp();
            });


            // get state ID with the lowest time stamp for exploration
            stateID sID = findExplorableState(explorableLeaves);

            if (completed)
                break;

            // get the time stamp of the state
            Time t = statesByID.find(sID)->second.getTimeStamp();

            // remove the leaf states with higher time stamp
            explorableLeaves.erase(std::remove_if(explorableLeaves.begin(), explorableLeaves.end(),
                                                  [this, t](stateID sID) {
                                                      return statesByID.find(sID)->second.getTimeStamp() > t;
                                                  }), explorableLeaves.end());

            // sort states by the number of dispatched jobs
            std::sort(explorableLeaves.begin(), explorableLeaves.end(), [this](stateID a, stateID b) {
                return statesByID.find(a)->second.getNumberOfDispatchedJobs() <
                       statesByID.find(b)->second.getNumberOfDispatchedJobs();
            });

            // get state ID with the lowest time stamp and the number of dispatched jobs for exploration
            sID = findExplorableState(explorableLeaves);





            //get the state
            state<Time> s = statesByID.find(sID)->second;

            //check the timeout
            checkTimeout();

            if (aborted)
                break;

            exploreState(s);

        } while (numDispatchedJob < jobs.size());


    }

    void makeInitialStates() {
		// Estimate the number of states and preallocate memory accordingly.
		statesByID.reserve(jobs.size() * 2);
        // use a set to avoid duplicate event times
        std::set<Time> eventTimes;
        if (systemEvents.isAllEvent()) {
            for (long long i = 1; i <= tools::observationWindow; i++) {
                eventTimes.emplace(i);
            }
        } else {
            if (systemEvents.isArrivalEvent()) {
                for (auto &s: jobs) {
                    if (s.getArrival().from() != 0)
                        eventTimes.emplace(s.getArrival().from());
                    if (s.getArrival().until() != 0)
                        eventTimes.emplace(s.getArrival().until());
                }
            }
            if (systemEvents.getTimes().size() > 0) {
                for (auto &t: systemEvents.getTimes()) {
                    if (t != 0)
                        eventTimes.emplace(t);
                }
            }

        }
        std::vector<Time> eventSet(eventTimes.begin(), eventTimes.end());

        std::sort(eventSet.begin(), eventSet.end());

        state<Time> s(resourceSet, eventSet, systemEvents.isCompletionEvent());
        statesByID.emplace(numStates, s);
//        statesByKey.emplace(s.getKey(), s);
        transitionStructure.addNode(-1, s.getTimeStamp(), s.getStateLabel(), "");
        numStates++;
    }

    // explore a state for possible transitions
    void exploreState(const state<Time> &s) {
        log<LOG_INFO>("Checking state %1%") % s;
        readyQueues queues = makeReadyQueues(s);
        for (const auto &q: queues) {
            // convert the vector of job IDs to an actual queue
            auto rq = queue<Time>(jobsByID, q);
#ifdef RESOURCE_ACCESS
            // make available resource set based on the selected ready queue
            auto rangeOfAvailableResources = makeAvailableResourcesMap(rq, s);
            auto allAvailableResourcesCombinations = makeAllCombinationsOfAvailableResources(rangeOfAvailableResources);
            for (auto availableResources: allAvailableResourcesCombinations) {
                // call scheduler to select a job based on the scheduling policy from the possible ready queues
                auto selectedJob = schedulingPolicy.callScheduler(rq, availableResources, s.getTimeStamp());

                if (selectedJob == std::nullopt) {
                    log<LOG_INFO>("No job is selected from the ready queue");
                    // we have to do a time transition
                    // get the next event time
                    if (transitionStructure.hasEdge(s.getStateID(),
                                                    std::to_string(s.getNextEventTime() - s.getTimeStamp()))) {
                        log<LOG_INFO>("Time transition already exists");
                        continue;
                    } else {
                        auto nextEventTime = s.getNextEventTime();
                        if (!beNaive) {
                            // if we are not in naive mode,
                            // we apply the peeking strategy to implement scheduling event abstraction
							// create a temp copy of the state to avoid modifying the original state
							state<Time> tempState(s);
                            nextEventTime = peekState(queues, tempState);
                        }
                        timeTransition(s, nextEventTime);
                    }
                } else {
                    auto selectedJobID = selectedJob.value();
                    // we have a job to dispatch
                    // we need to check if the job is already dispatched
                    if (transitionStructure.hasEdge(s.getStateID(), selectedJobID.string())) {
                        log<LOG_INFO>("The transition already proceeded");
                        continue;
                    } else {
                        // we have a new dispatch transition
                        dispatchJob(s, selectedJobID);
                    }
                }
            }
#else
			// call scheduler to select a job based on the scheduling policy from the possible ready queues
			// we do not need to consider the available resources in this case, so we pass an empty map
			auto selectedJob = schedulingPolicy.callScheduler(rq, {}, s.getTimeStamp());

			if (selectedJob == std::nullopt) {
				log<LOG_INFO>("No job is selected from the ready queue");
				// we have to do a time transition
				// get the next event time
				if (transitionStructure.hasEdge(s.getStateID(),
												std::to_string(s.getNextEventTime() - s.getTimeStamp()))) {
					log<LOG_INFO>("Time transition already exists");
					continue;
				} else {
					auto nextEventTime = s.getNextEventTime();
					if (!beNaive) {
						// if we are not in naive mode,
						// we apply the peeking strategy to implement scheduling event abstraction
						state<Time> tempState(s);
						nextEventTime = peekState(queues, tempState);
					}
					timeTransition(s, nextEventTime);
				}
			} else {
				auto selectedJobID = selectedJob.value();
				// we have a job to dispatch
				// we need to check if the job is already dispatched
				if (transitionStructure.hasEdge(s.getStateID(), selectedJobID.string())) {
					log<LOG_INFO>("The transition already proceeded");
					continue;
				} else {
					// we have a new dispatch transition
					dispatchJob(s, selectedJobID);
				}
			}
#endif


        }

    }

    void dispatchJob(const state<Time> &s, jobID sid) {
        log<LOG_INFO>("Dispatching job %1%") % sid;
		// get the job from the job ID
		const job<Time> &job = *(jobRefsByID.find(sid)->second);
		//calculate the earliest and latest finish time of the job
		Interval<Time> ftimes = job.getCost() + s.getTimeStamp();

        state<Time> newState(s, numStates, job, indexOf(job),
							 job.getAssignedProcessorSet(), ftimes);
        if (!beNaive) {
            if (tryToMergeStates(newState, s.getStateID(), sid.string())) {
                updateResponseTime(sid, ftimes);
                return;
            }
        }
        log<LOG_INFO>("New state is created");
        statesByID.emplace(numStates, newState);
        transitionStructure.addNode(s.getStateID(), newState.getTimeStamp(), newState.getStateLabel(),
                                    sid.string());
        numStates++;

        updateResponseTime(sid, ftimes);

    }

    void timeTransition(const state<Time> &s, Time nextEventTime) {
        // check if the time transition already exists
        if (transitionStructure.hasEdge(s.getStateID(),
                                        std::to_string(nextEventTime - s.getTimeStamp()))) {
            log<LOG_INFO>("Time transition already exists");
            return;
        }

        log<LOG_INFO>("Time transition to %1% from state %2%") % nextEventTime % s.getStateID();
        state<Time> newState(s, numStates, nextEventTime);
        if (!beNaive) {
            if (tryToMergeStates(newState, s.getStateID(), std::to_string(nextEventTime - s.getTimeStamp()))) {
                log<LOG_INFO>("Merged with an existing state");
                return;
            }
        }
        statesByID.emplace(numStates, newState);
//        statesByKey.emplace(newState.getKey(), newState);
        transitionStructure.addNode(s.getStateID(), newState.getTimeStamp(), newState.getStateLabel(),
                                    std::to_string(nextEventTime - s.getTimeStamp()));
        numStates++;


    }

	bool tryToMergeStates(const state<Time> &s, stateID parentID, std::string transitionLabel) {
		// get all leaf states
		auto leaves = transitionStructure.getLeaves();

		for (const auto &id: leaves) {
			// check if the state can be merged with any of the leaf states
			auto it = statesByID.find(id);
			if (it != statesByID.end() && it->second.tryToMerge(s)) {
				// we have merged the states
				// we need to update the transition structure
				transitionStructure.updateNodeLabel(it->second.getStateID(), it->second.getStateLabel());
				// make a new edge from the parent of s to the existing state
				transitionStructure.addEdge(parentID, it->second.getStateID(), transitionLabel);
				log<LOG_INFO>("Merged with an existing state: %1%") % it->second.getStateLabel();
				return true;
			}
		}
		return false;
	}

    Time peekState(const readyQueues &queues, state<Time> &fromState) {
        // take a quick look at the next state to see if it makes different ready queues or dispatches a different job
        // if it does, we need to create a new state
        // otherwise, we can ignore it

        // 1. found all the jobs that will be dispatched by the current ready queues
        std::unordered_set<jobID> dispatchedJobs;
		// reserve memory for the dispatched jobs
		dispatchedJobs.reserve(jobs.size());
        for (const auto &q: queues) {
            auto rq = queue<Time>(jobsByID, q);
            auto rangeOfAvailableResources = makeAvailableResourcesMap(rq, fromState);
            auto allAvailableResourcesCombinations = makeAllCombinationsOfAvailableResources(rangeOfAvailableResources);
            for (const auto &availableResources: allAvailableResourcesCombinations) {
                auto selectedJob = schedulingPolicy.callScheduler(rq, availableResources, fromState.getTimeStamp());
                if (selectedJob != std::nullopt) {
                    dispatchedJobs.emplace(selectedJob.value());
                }
            }
        }

        log<LOG_INFO>(">> Peeking the next state in time %1%") % (fromState.getNextEventTime());
        log<LOG_INFO>("-------------------------");
//        state<Time> newState(fromState, numStates, fromState.getNextEventTime());
		// update the time stamp of the current state
		auto nextEventTime = fromState.getNextEventTime();
		fromState.updateEventSet(nextEventTime);
        readyQueues nextStateQueues = makeReadyQueues(fromState);

        // 2. found all the jobs that will be dispatched by the next ready queues
        std::unordered_set<jobID> nextDispatchedJobs;
		// reserve memory for the dispatched jobs
		nextDispatchedJobs.reserve(jobs.size());
        for (const auto &q: nextStateQueues) {
            auto rq = queue<Time>(jobsByID, q);
            auto rangeOfAvailableResources = makeAvailableResourcesMap(rq, fromState);
            auto allAvailableResourcesCombinations = makeAllCombinationsOfAvailableResources(rangeOfAvailableResources);
            for (const auto &availableResources: allAvailableResourcesCombinations) {
                auto selectedJob = schedulingPolicy.callScheduler(rq, availableResources, fromState.getTimeStamp());
                if (selectedJob != std::nullopt) {
                    nextDispatchedJobs.emplace(selectedJob.value());
                }
            }
        }

        // 3. compare the two sets of dispatched jobs
        if (nextDispatchedJobs != dispatchedJobs || nextStateQueues != queues) {
            log<LOG_INFO>("-------------------------");
            return fromState.getTimeStamp();
        } else {
            return peekState(queues, fromState);
        }
    }

    // update number of dispatched jobs
    void updateNumDispatchedJobs(const std::vector<stateID> &stateIDs) {
        for (auto &sID: stateIDs) {
            auto s = statesByID.find(sID);
            numDispatchedJob = std::min(numDispatchedJob, s->second.getNumberOfDispatchedJobs());
        }
    }

    // find explorable state
    stateID findExplorableState(std::vector<stateID> &stateIDs) {
        // find the first explorable state
        for (int i = 0; i < stateIDs.size(); i++) {
            auto s = statesByID.find(stateIDs[i]);
            if (s->second.getNumberOfDispatchedJobs() != jobs.size()) {
                return stateIDs[i];
            }
        }
        completed = true;
        return -1;
    }

    // find explorable states
std::vector<stateID> findExplorableStates(std::vector<stateID> &stateIDs) {
    std::vector<stateID> explorableStates;
    explorableStates.reserve(stateIDs.size());
    for (auto &sID: stateIDs) {
        auto s = statesByID.find(sID);
        if (s->second.getNumberOfDispatchedJobs() != jobs.size()) {
            explorableStates.emplace_back(sID);
        }
    }
    if (explorableStates.empty()) {
        completed = true;
    }
    return explorableStates;
}

    // update the response time of the job
    void updateResponseTime(jobID sid, Interval<Time> rt) {
        auto it = responseTimes.find(sid);
        if (it == responseTimes.end()) {
            responseTimes.emplace(sid, rt);
        } else {
            it->second |= rt;
        }

        if (jobsByID.find(sid)->second.exceedsDeadline(rt.upto())) {
            log<LOG_WARNING>("Job %1% exceeds deadline") % sid;
            aborted = true;
        }
    }

    readyQueues makeReadyQueues(const state<Time> &s) {
        readyQueues queues;
        readyQueue possibleReadyJobs;
        readyQueue certainReadyJobs;
        readyQueue certainReleasedJobs;
        readyQueue allReadyJobs;

        allReadyJobs.reserve(jobs.size());
        certainReadyJobs.reserve(jobs.size());
        certainReleasedJobs.reserve(jobs.size());
        possibleReadyJobs.reserve(jobs.size());

        for (const job<Time> &seg: jobsByWin.lookup(s.getTimeStamp())) {
            // check if the job is already dispatched
            log<LOG_DEBUG>("Checking job %1%") % seg.getID();
            if (s.isDispatched(indexOf(seg))) {
                log<LOG_DEBUG>(" -> Job %1% is already dispatched") % seg.getID();
                continue;
            } else if (seg.exceedsDeadline(s.getTimeStamp() + seg.getCost().max())) {
				log<LOG_DEBUG>(" -> Job %1% missed its deadline") % seg.getID();
				aborted = true;
			}

            // check if job is possibly released
            if (s.getTimeStamp() >= seg.getArrival().from()) {
                // check if the processor is available
                if (s.getProcessorAvailability(seg.getAssignedProcessorSet(), 1).min() <= s.getTimeStamp()) {
                    allReadyJobs.emplace_back(seg.getID());
                } else {
                    log<LOG_DEBUG>(" -> Job %1%'s resource is not available") % seg.getID();
                }
            } else {
                log<LOG_DEBUG>(" -> Job %1% is not released") % seg.getID();
                log<LOG_DEBUG>(" -> No need to check further");
                continue;
            }

            // make queue of certain ready jobs
            if (s.getTimeStamp() >= seg.getArrival().until()) {
                // check if the processor is available
                if (s.getProcessorAvailability(seg.getAssignedProcessorSet(), 1).max() <= s.getTimeStamp()) {
                    certainReadyJobs.emplace_back(seg.getID());
                } else if (s.getProcessorAvailability(seg.getAssignedProcessorSet(), 1).min() <= s.getTimeStamp()) {
                    // jobs that certainly released but their resource is not certainly available
                    certainReleasedJobs.emplace_back(seg.getID());
                }
            }
        }

        // make queue of possible ready jobs
        // possible ready jobs are jobs that are not in certainly ready jobs
        readyQueue tempQueue;
        tempQueue.reserve(allReadyJobs.size());
        std::set_difference(allReadyJobs.begin(), allReadyJobs.end(), certainReadyJobs.begin(),
                            certainReadyJobs.end(), std::back_inserter(tempQueue));

        std::set_difference(tempQueue.begin(), tempQueue.end(), certainReleasedJobs.begin(),
                            certainReleasedJobs.end(), std::back_inserter(possibleReadyJobs));

        // make ready queues (union of possible ready jobs and certainly ready jobs)
        for (auto q: makePowerset(possibleReadyJobs)) {
            readyQueue temp;
            temp.reserve(certainReadyJobs.size() + q.size());
            temp.insert(temp.end(), certainReadyJobs.begin(), certainReadyJobs.end());
            temp.insert(temp.end(), q.begin(), q.end());
            queues.emplace_back(temp);
        }

        // add certainly released jobs to the queues
        readyQueues tempQueues;
        tempQueues.reserve(queues.size());
        for (auto &q: queues) {
            readyQueue temp;
            temp.reserve(q.size() + certainReleasedJobs.size());
            temp.insert(temp.end(), q.begin(), q.end());
            temp.insert(temp.end(), certainReleasedJobs.begin(), certainReleasedJobs.end());
            tempQueues.emplace_back(temp);
        }

        std::copy_if(tempQueues.begin(),
                     tempQueues.end(),
                     std::back_inserter(queues),
                     [&](auto q) { return q.size() != 0 && !std::count(queues.begin(), queues.end(), q); });

#ifdef DEBUG
        // print all possible ready queues
        log<LOG_INFO>("%1% Possible ready queues:") % queues.size();
        for (auto &q: queues) {
            std::string temp = "queue: {";
            for (auto &seg: q) {
                temp.append(seg.string());
                temp.append(", ");
            }
            log<LOG_INFO>("%1%}") % temp;
        }
#endif
        return queues;
    }


readyQueues makePowerset(readyQueue &queue) {
    readyQueues powerset;
    powerset.reserve(1 << queue.size()); // Reserve space for all subsets
    powerset.push_back({}); // Add the empty set

        for (std::size_t k = 0; k <= queue.size(); ++k) {
            for_each_combination(queue.begin(), queue.begin() + k, queue.end(),
                                 [&](auto first, auto last) {
                                     readyQueue newQueue;
                                     if (first != last) {
                                         std::copy(first, last, std::back_inserter(newQueue));
                                         powerset.push_back(newQueue);
                                     }
                                     return false;
                                 });
        }
        return powerset;
    }

    std::unordered_map<std::string, Interval<Time>>
    makeAvailableResourcesMap(queue<Time> rq, state<Time> currentState) {
        // make a map of available resources
        // key: resource name (e.g. "P1")
        // value: interval of possible available resources (e.g. [1, 10] 1: certainly available, 10: possibly available)
        std::unordered_map<std::string, Interval<Time>> availableResources;
        for (int processors_ID = 0; processors_ID < processors.size(); processors_ID++) {
            auto name = processors[processors_ID].getName();
            // find number of possibly available resources
            // Since job is on the queue, one of the resources is certainly available
            unsigned int certainlyAvailableResources = 0;
            unsigned int possiblyAvailableResources = 0;
            for (auto &seg: rq.getElementsByID()) {
                auto it = jobsByID.find(seg);
                if (it != jobsByID.end()) {
                    auto seg_processors_ID = it->second.getAssignedProcessorSet();
                    if (seg_processors_ID == processors_ID && certainlyAvailableResources == 0) {
                        certainlyAvailableResources++;
                        possiblyAvailableResources++;
                        break;
                    }
                } else {
                    log<LOG_ERROR>("Job %1% not found") % seg;
                    assert(false);
                }


            }


            for (int i = certainlyAvailableResources + 1; i <= processors[processors_ID].getNumCores(); i++) {
                auto availability = currentState.getProcessorAvailability(processors_ID, i);
                if (availability.max() <= currentState.getTimeStamp()) {
                    certainlyAvailableResources++;
                    possiblyAvailableResources++;
                } else if (availability.min() <= currentState.getTimeStamp() &&
                           currentState.getTimeStamp() < availability.max()) {
                    possiblyAvailableResources++;
                }
            }

            availableResources[name] = Interval<Time>(certainlyAvailableResources, possiblyAvailableResources);
        }
        return availableResources;
    }

    std::vector<std::unordered_map<std::string, unsigned int>> makeAllCombinationsOfAvailableResources(
            std::unordered_map<std::string, Interval<Time>> availableResources) {
        // make a vector of all possible combinations of number of available resources
        std::vector<std::unordered_map<std::string, unsigned int>> combinationsMap;
        // make a vector of possible number of available resources range
        // e.g. [2, 10] 2: certainly available, 10: possibly available
        // 10 - 1 = 9 possible number of available resources
        std::vector<unsigned int> availablity;
        for (auto &resource: availableResources) {
            availablity.push_back(resource.second.max() - resource.second.min());
        }

        std::vector<unsigned int> combination;
        std::vector<std::vector<unsigned int>> combinations;
        combinations = makeAllCombinations(availablity, combination, combinations, 0);

        // make a map of all possible combinations of number of available resources
        // key: resource name (e.g. "P1")
        // value: number of available resources (e.g. 3)
        for (auto &c: combinations) {
            std::unordered_map<std::string, unsigned int> combinationMap;
            int i = 0;
            for (auto &resource: availableResources) {
                combinationMap[resource.first] = resource.second.min() + c[i];
                i++;
            }
            combinationsMap.push_back(combinationMap);
        }


        // print all possible combinations of number of available resources
        std::string temp;
        temp.append("combinationMap: {");
        for (auto &c: combinationsMap) {
            temp.append(" {");
            for (auto &i: c) {
                temp.append(i.first);
                temp.append(": ");
                temp.append(std::to_string(i.second));
                temp.append(", ");
            }
            temp.append("}");
        }
        log<LOG_DEBUG>("%1%}") % temp;

        return combinationsMap;
    }

    std::vector<std::vector<unsigned int>>
    makeAllCombinations(std::vector<unsigned int> availablity, std::vector<unsigned int> &combination,
                        std::vector<std::vector<unsigned int>> &combinations,
                        unsigned int index) {
        if (availablity.size() == 0) {
            log<LOG_DEBUG>("No available resources");
            return combinations;
        }

        for (int i = 0; i <= availablity[index]; ++i) {
            if (index == 0) {
                combination.clear();
                for (int j = 0; j < availablity.size(); ++j) {
                    combination.push_back(0);
                }
            }

            combination[index] = i;
            if (index == availablity.size() - 1) {
                combinations.push_back(combination);
            } else {
                makeAllCombinations(availablity, combination, combinations, index + 1);
            }

        }

        return combinations;
    }


    void checkTimeout() {
        if (timer > timeout) {
            log<LOG_ERROR>("Timeout!");
            aborted = true;
            timedOut = true;
        }
    }

    // memory used by the process
    double getRAM() {
#ifndef _WIN32
        struct rusage u;
        long memUsed = 0;
        if (getrusage(RUSAGE_SELF, &u) == 0)
            memUsed = u.ru_maxrss;
        return (double) memUsed / 1024.0;
#else
        // TODO: check memory usage on windows
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
        return (double) virtualMemUsedByMe / 1024.0;
#endif
    }

    std::stringstream getFormattedOutput() {
        std::stringstream ss;
        TextTable t('-', '|', '+');
        t.add(" Output file ");
        t.add(" Schedulable? ");
        t.add(" #States ");
        t.add(" CPU time(s) ");
        t.add(" memory (MB) ");
        t.add(" Timeout? ");
        t.add(" #CPUs ");
        t.endOfRow();
        t.add(jobFile);
        t.add(completed ? "Yes" : "No");
        t.add(std::to_string(numStates));
        t.add(std::to_string(double(timer)));
        t.add(std::to_string(getRAM()));
        t.add(timedOut ? "Yes" : "No");
        t.add(std::to_string(resourceSet.size()));
        t.endOfRow();

        t.setAlignment(3, TextTable::Alignment::RIGHT);
        ss << t;

        return ss;
    }

    std::stringstream getRawOutput() {
        std::stringstream ss;
        ss << jobFile;
        ss << ",  " << (completed ? "Yes" : "No");
        ss << ",  " << numStates;
        ss << ",  " << double(timer);
        ss << ",  " << getRAM();
        ss << ",  " << (timedOut ? "Yes" : "No");
        ss << ",  " << resourceSet.size() << std::endl;
        return ss;
    }

    void makeCSVFile() {
        std::ofstream csvFile;
        std::string fileName = jobFile.substr(0, jobFile.find_last_of("."));
        csvfile rtCsv(fileName.append(".csv"), false, ",");
        // add header
        rtCsv << "Task ID" << "Job ID" << "BCCT" << "WCCT" << "BCRT" << "WCRT" << endrow;

        // add response and completion times
        for (auto &seg: jobs) {
            auto rt = responseTimes.find(seg.getID());
            if (rt == responseTimes.end()) {
                rtCsv << seg.getID().task << seg.getID().job << -1 << -1 << -1 << -1 << endrow;
            } else {
                rtCsv << seg.getID().task << seg.getID().job << rt->second.min() << rt->second.max()
                      << rt->second.min() - seg.getArrival().min() << rt->second.max() - seg.getArrival().max()
                      << endrow;
            }
        }
    }

    void makeDotFile() {
        std::string fileName = jobFile.substr(0, jobFile.find_last_of("."));
        transitionStructure.generateDotFile(fileName.append(".dot"));
    }

	Time maxDeadline(const jobSet &jobs) {
		Time dl = 0;
		for (auto &j: jobs) {
			dl = std::max(dl, j.getDeadline());
		}
		return dl;
	}

	std::size_t indexOf(const job<Time> &j) const {
		// make sure that the job is part of the workload
		// and catch the case where the job is not part of the workload,
		// but the user tries to access it anyway
		auto index = (std::size_t) (&j - &(jobs[0]));
		try {
			jobs.at(index);
		} catch (std::out_of_range &e) {
			std::cerr << "Job " << j << " not found in workload." << std::endl;
			std::abort();
		}
		return index;
	}


};

#endif //TRANSITION_SYSTEM_HPP
