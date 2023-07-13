#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <vector>
#include <memory>


class processor {
private:
    /* data */
    std::string name;
    unsigned long processorID;
    unsigned int numCores;

public:
    processor() = default;

    processor(std::string name, unsigned long processorID, unsigned int numCores)
            : name(name), processorID(processorID), numCores(numCores) {
    }

    // Encapsulation
    void setName(std::string name) {
        this->name = name;
    }

    void setProcessorID(unsigned long processorID) {
        this->processorID = processorID;
    }

    void setNumCores(unsigned int numCores) {
        this->numCores = numCores;
    }

    std::string getName() const {
        return name;
    }

    unsigned long getProcessorID() const {
        return processorID;
    }

    unsigned int getNumCores() const {
        return numCores;
    }
};


#endif
