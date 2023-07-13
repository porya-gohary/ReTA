#include <iostream>
#include <string>
#include "time.hpp"
#include "OptionParser.h"
#include "formattedLog.hpp"
#include "tools.hpp"
#include "dag.hpp"
#include "transitionSystem.hpp"
#include "models/systemModel.hpp"


// command line options
static unsigned int numProcessors = 1;
static unsigned int verbose = 0;
static std::string outputFile = "out.csv";
const std::string version = "%prog 0.2\nCopyright (C) 2023 Pourya Gohari\n"
                            "License GPLv3+: GNU GPL version 3 or later "
                            "<http://gnu.org/licenses/gpl.html>.\n"
                            "This is free software: you are free to change and redistribute it.\n"
                            "There is NO WARRANTY, to the extent permitted by law.";
const std::string usage = "usage: %prog [OPTIONS]...";
const std::string desc = "ReDo: A versatile framework for modeling and analyzing arbitrary online scheduling policies for real-time systems";
static bool wantNaive;
static bool wantDense = false;
static double timeout = 14400;
static bool rawFormat = false;

// global variables


// make analysis
template<class Time>
transitionSystem<Time> makeAnalysis(tools::inputs<Time> &inputs) {
    // make transition system
    if (wantNaive) {
        return transitionSystem<discreteTime>::constructNaively(outputFile, inputs.segments, numProcessors, timeout);
    } else {
        return transitionSystem<discreteTime>::construct(outputFile, inputs.segments, numProcessors, timeout);
    }

}

// make analysis
template<class Time>
transitionSystem<Time>
makeAnalysis(std::vector<job<Time>> &inputs, events<Time> systemEvents, std::vector<processor> processors) {
    // make transition system
    if (wantNaive) {
        return transitionSystem<Time>::constructNaively(outputFile, inputs, systemEvents, processors, timeout);
    } else {
        return transitionSystem<Time>::construct(outputFile, inputs, systemEvents, processors, timeout);
    }

}

int main(int argc, char **argv) {
    auto parser = optparse::OptionParser();

    parser.description(desc);
    parser.usage(usage);
    parser.version(version);
    parser.add_help_option(false);
    parser.add_version_option(false);

    parser.add_option("-l", "--time-limit").dest("timeout").type("double")
            .help("maximum CPU time allowed (in seconds, zero means no limit)")
            .set_default("14400");

    parser.add_option("-n", "--naive").dest("naive").set_default("0")
            .action("store_const").set_const("1")
            .help("use the naive exploration method (default: false)");

    parser.add_option("-r", "--raw").dest("raw").set_default("0")
            .action("store_const").set_const("1")
            .help("print output without formatting (default: false)");

    parser.add_option("-o", "--output").dest("outputFile")
            .help("name of the output file (default: out.csv)")
            .set_default("out.csv");

    /*
    VERBOSE MESSAGE DETAIL:
     0 ->   LOG_NOTHING, 1 ->   LOG_CRITICAL, 2 ->   LOG_ERROR, 3 ->   LOG_WARNING, 4 ->   LOG_INFO, 5 ->   LOG_DEBUG
    */
    parser.add_option("-e", "--verbose").dest("verbose").set_default("0")
            .help("print log messages [0-5]" "(default: 0)");

    parser.add_option("-v", "--version").action("version").help("show program's version number and exit");
    parser.add_option("-h", "--help").action("help").help("show this help message and exit");

    // Put args on variables
    optparse::Values options = parser.parse_args(argc, argv);

    timeout = options.get("timeout");

    verbose = options.get("verbose");
    log_impl::GLOBAL_LEVEL = verbose;

    wantNaive = options.get("naive");

    rawFormat = options.get("raw");

    outputFile = (const std::string) options.get("outputFile");


    // read input files
//    auto inputs = processFiles<model::time>();
    model::defineEvents();
    model::defineProcessors();
    model::defineTasks();
    model::defineSegments();

    auto inputs = tools::generateSegments<model::time>(model::tasks, model::segments);

    // make analysis
    auto ts = makeAnalysis<model::time>(inputs, model::systemEvents, model::processors);

    if (rawFormat)
        std::cout << ts.getRawOutput().str();
    else
        std::cout << ts.getFormattedOutput().str();

    ts.makeCSVFile();
    ts.makeDotFile();
}
