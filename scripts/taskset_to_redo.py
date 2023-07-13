#!/usr/bin/env python3
"""
A script to convert task set format to CPP files of ReDo.

Usage:
    task_set_to_redo                [options]

Options:
    --task-set FILE, -t FILE             job set csv file [default: example.csv]
    --version, -v                       show version and exit
    --help, -h                          show this message
"""

from docopt import docopt
from time import time
import sys
import os
import csv
import lib.task as task
import lib.template as template
from math import ceil, floor, gcd

n_PE = 4  # N
n_tasks = 10  # M


def read_csv(file):
    taskset = []
    try:
        with open(file, 'r') as read_obj:
            # csv_reader = csv.reader(read_obj)
            csv_dict_reader = csv.DictReader(read_obj)
            # Iterate over each row in the csv using reader object and make tasks
            for row in csv_dict_reader:
                taskset.append(
                    task.task(name=row['Name'], jitter=int(row['Jitter']), bcet=int(row['BCET']), wcet=int(row['WCET']),
                              period=int(row['Period']), deadline=int(row['Deadline']), pe=int(row['PE'])))

        for t in taskset:
            if t.period >= 32768:
                #            print("WARNING: Period is too high for UPPAAL")
                for i in range(0, 3):
                    if t.period >= 32768:
                        t.period = int(t.period / 10)
                        t.wcet = int(t.wcet / 10)
                        t.bcet = int(t.bcet / 10)
                        t.deadline = int(t.deadline / 10)
                        t.jitter = int(t.jitter / 10)
                    else:
                        break
        return taskset
    except Exception as e:
        print(e)
        print("ERROR: reading taskset is not possible")
        sys.exit(1)


def make_redo_file(taskset, file_name):
    tasks = ""
    for t in taskset:
        tasks += "\t\ttasks.emplace_back(\"" + t.name + "\", taskCounter++, " + str(t.jitter) + ", " + str(
            t.period) + ", Interval<time>(" + str(t.bcet) + "," + str(t.wcet) + "), " + str(
            t.deadline) + ", " + str(0) + ", " + str(0) + ");" + "\n"
    template.cpp_model_template[22] = tasks

    procs = "\t\tprocessors.emplace_back(\"p1\", processorCounter++, " + str(n_PE) + " );\n"
    template.cpp_model_template[17] = procs

    try:
        cpp_file_name = file_name + ".hpp"
        cpp_file = open(cpp_file_name, "w+")
        cpp_file.writelines(template.cpp_model_template)
        cpp_file.close()
    except Exception as e:
        print(e)
        print("ERROR: writing to system model file is not possible")
        sys.exit(1)


def main():
    args = docopt(__doc__, version='0.2.0')
    task_set = read_csv(args['--task-set'])
    global n_tasks
    n_tasks = len(task_set)
    task_set_name = os.path.splitext(os.path.basename(args['--task-set']))[0]
    make_redo_file(task_set, task_set_name)

if __name__ == "__main__":
    main()
