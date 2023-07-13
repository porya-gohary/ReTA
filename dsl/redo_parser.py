#!/usr/bin/env python3
"""
ReDo language parser

Usage:
    redo_parser <file>                  [options]

Options:
    --version, -v                       show version and exit
    --help, -h                          show this message
"""
from textx import metamodel_from_file
from template import (cpp_model_template, cpp_scheduler_template)
from docopt import docopt
from pathlib import Path
import os

read_from_file = False


def interpret(model):
    """Interpret the model and generate the C++ code."""

    # check for rules
    check_rules(model)

    f = open("systemModel.hpp", "w")

    # write time model to header file
    if model.time_model != None and model.time_model.model == "dense":
        cpp_model_template[7] = "\ttypedef denseTime time;\n\n"
    else:
        cpp_model_template[7] = "\ttypedef discreteTime time;\n\n"

    # write events to header file
    events = []
    event_all = False
    event_arrival = False
    event_completion = False

    event_temp = "\t\tsystemEvents = events<time>("
    for e in model.events.event:
        if e == "all":
            event_all = True
        elif e == "arrival":
            event_arrival = True
        elif e == "completion":
            event_completion = True
        else:
            events.append(str(e))

    if event_all:
        event_temp += "true, "
    else:
        event_temp += "false, "

    if event_arrival:
        event_temp += "true, "
    else:
        event_temp += "false, "

    if event_completion:
        event_temp += "true"
    else:
        event_temp += "false"

    event_temp += ", {"
    if len(events) > 0:
        for e in events:
            event_temp += "time(" + e + "), "
    event_temp += "} );\n"

    cpp_model_template[11] = event_temp

    # write processors to header file
    procs = ""
    procsID = []
    for r in model.resources.resource:
        procs += "\t\tprocessors.emplace_back(\"" + r.name + \
                 "\", processorCounter++, " + str(r.ncores) + ");" + "\n"
        procsID.append(r.name)

    cpp_model_template[17] = procs

    tasks = ""
    jobs = ""
    for t in model.workloads.workload:
        if t.type == "task":
            tasks += "\t\ttasks.emplace_back(\"" + t.name + "\", taskCounter++, " + str(t.jitter) + ", " + str(
                t.period) + ", Interval<time>(" + str(t.cost_low) + "," + str(t.cost_up) + "), " + str(
                t.deadline) + ", " + str(procsID.index(t.processor.name)) + ", " + str(t.priority) + ");" + "\n"

        elif t.type == "job":
            jobs += "\t\tsegments.emplace_back(\"" + t.name + "\", taskCounter++, 0 , Interval<time>(" + str(
                t.arrival_low) + "," + str(t.arrival_up) + "), Interval<time>(" + str(t.cost_low) + "," + str(
                t.cost_up) + "), " + str(t.deadline) + ", " + str(procsID.index(t.processor.name)) + ", " + str(
                t.priority) + ");" + "\n"

    cpp_model_template[22] = tasks
    cpp_model_template[25] = jobs
    f.writelines(cpp_model_template)
    f.close()

    # make cpp header for scheduler
    f = open("scheduler.hpp", "w")

    # call scheduler commands handler
    scheduler_commands_handler(model.scheduler.commands)

    if read_from_file:
        f.writelines(imported_file.read())
        print("imported file")
        f.close()
    else:
        f.writelines(cpp_scheduler_template)
        f.close()


def scheduler_commands_handler(commands, line_tabs="\t\t"):
    """general commands handler"""

    for c in commands:
        if c.__class__.__name__ == "Queue_command":
            if c.command is not None:
                if c.command.name == "sort":
                    cpp_scheduler_template[14] += line_tabs + "readyQueue.sort(sortKey::" + \
                                                  c.command.key + ");" + "\n"
                else:
                    print("Error: queue command not found!")
            elif c.func is not None:
                if c.func.name == "length":
                    cpp_scheduler_template[14] += line_tabs + \
                                                  "readyQueue.length();" + "\n"
                elif c.func.name == "at":
                    if c.func.index == str:
                        print("Error: index must be an integer!")
                        exit(1)
                    else:
                        if c.func.index.ref is not None:
                            # if index is a variable
                            cpp_scheduler_template[14] += line_tabs + "readyQueue.at(" + \
                                                          c.command.func.ref.name + ");" + "\n"
                        else:
                            # if index is an integer
                            cpp_scheduler_template[14] += line_tabs + "readyQueue.at(" + \
                                                          str(int(c.func.index.value)) + ");" + "\n"
                else:
                    print("Error: queue function not found!")
            else:
                print("Error: queue command not found!")

        elif c.__class__.__name__ == "Return":
            if c.value is not None:
                cpp_scheduler_template[14] += line_tabs + \
                                              "return " + str(c.value.name) + ";" + "\n"
            elif c.func is not None:
                cpp_scheduler_template[14] += line_tabs + "return readyQueue." + \
                                              c.func.func.name + "();" + "\n"
            elif c.nul is not None:
                cpp_scheduler_template[14] += line_tabs + \
                                              "return std::nullopt;" + "\n"
            else:
                print("Error: cannot return value!")
                exit(1)

        elif c.__class__.__name__ == "Variable_declaration" or c.__class__.__name__ == "Variable_assignment":
            variable_handler(c, line_tabs)

        elif c.__class__.__name__ == "Use_file":
            global imported_file
            global read_from_file
            imported_file = open(c.file, "r")
            read_from_file = True
            break

        elif c.__class__.__name__ == "If_statement":
            condition_handler(c, line_tabs)


def variable_handler(command, line_tabs):
    """variable declaration and assignment handler"""
    if command.__class__.__name__ == "Variable_declaration":
        if command.rhs is not None:
            if command.rhs.__class__.__name__ == "Queue_function":
                if command.rhs.func.name == "length":
                    cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                                  " " + command.var.name + " = readyQueue.length();" + "\n"
                elif command.rhs.func.name == "front().wcet":
                    cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                                  " " + command.var.name + " = segmentsByID.find(readyQueue.front())->second.getMaximalCost();" + "\n"
                elif command.rhs.func.name == "at":
                    print("Error: cannot copy ready queue objects!")
                    exit(1)
                elif command.rhs.func.name == "back":
                    print("Error: cannot copy ready queue objects!")
                    exit(1)
                elif command.rhs.func.name == "front":
                    print("Error: cannot copy ready queue objects!")
                    exit(1)

            elif command.rhs.__class__.__name__ == "Current_time_function":
                cpp_scheduler_template[14] += line_tabs + "Time" + \
                                              " " + command.var.name + " = currentTime;" + "\n"

            elif command.rhs.__class__.__name__ == "Resource_function":
                cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                              " " + command.var.name + \
                                              " = availableResources.find(\"" + str(
                    command.rhs.resource.name) + "\")->second;" + "\n"

            elif command.rhs.__class__.__name__ == "Task_function":
                # TODO: implement task function
                if command.rhs.func.name == "next_deadline":
                    pass
                elif command.rhs.func.name == "wcet":
                    pass

            elif command.rhs.__class__.__name__ == "Operation":
                cpp_scheduler_template[14] += line_tabs +command.var.var_type + \
                                              " " + command.var.name + " = " + operation_handler(command.rhs) + ";" + "\n"

            else:
                if command.rhs.ref is not None:
                    # if right hand side is a variable
                    if command.var.var_type != "string":
                        cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                                      " " + command.var.name + " = " + command.rhs.ref.name + ";" + "\n"
                    else:
                        cpp_scheduler_template[14] += line_tabs + "std::string" + \
                                                      " " + command.var.name + " = " + command.rhs.ref.name + ";" + "\n"
                elif command.rhs.value is not None:
                    # if right hand side is a value
                    if command.var.var_type != "string":
                        cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                                      " " + command.var.name + " = " + \
                                                      str(command.rhs.value) + \
                                                      ";" + "\n"
                    else:
                        cpp_scheduler_template[14] += line_tabs + "std::string" + \
                                                      " " + command.var.name + " = \"" + command.rhs.value + "\";" + "\n"
        else:
            cpp_scheduler_template[14] += line_tabs + command.var.var_type + \
                                          " " + command.var.name + ";" + "\n"

    elif command.__class__.__name__ == "Variable_assignment":
        if command.rhs.__class__.__name__ == "Queue_function":
            if command.rhs.func.name == "length":
                cpp_scheduler_template[14] += line_tabs + \
                                              command.ref.name + " = readyQueue.length();" + "\n"
            elif command.rhs.func.name == "front().wcet":
                cpp_scheduler_template[14] += line_tabs + \
                                              command.ref.name + " = segmentsByID.find(readyQueue.front())->second.getMaximalCost();" + "\n"
            elif command.rhs.func.name == "at":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.rhs.func.name == "back":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.rhs.func.name == "front":
                print("Error: cannot copy ready queue objects!")
                exit(1)
        elif command.rhs.__class__.__name__ == "Resource_function":
            cpp_scheduler_template[14] += line_tabs + command.ref.name + \
                                          " = availableResources.find(\"" + \
                                          str(command.rhs.resource.name) + "\")->second;" + "\n"
        elif command.rhs.__class__.__name__ == "Task_function":
            if command.rhs.func.name == "next_deadline":
                cpp_scheduler_template[14] += line_tabs + command.ref.name + " = " + \
                                              " getNextDeadline( \"" + command.rhs.task.name + \
                                              "\",currentTime);" + "\n"
            elif command.rhs.func.name == "wcet":
                cpp_scheduler_template[14] += line_tabs + command.ref.name + " = " + \
                                              " segmentsByID.at(getJobID(\"" + command.rhs.task.name + \
                                              "\")).getMaximalCost();" + "\n"
        elif command.rhs.__class__.__name__ == "Current_time_function":
            cpp_scheduler_template[14] += line_tabs + \
                                          command.ref.name + " = currentTime;" + "\n"

        elif command.rhs.__class__.__name__ == "Operation":
            cpp_scheduler_template[14] += line_tabs + command.ref.name + \
                                          " = " + operation_handler(command.rhs) + ";" + "\n"
        else:
            if command.rhs.ref is not None:
                # if right hand side is a variable
                cpp_scheduler_template[14] += line_tabs + \
                                              command.ref.name + " = " + command.rhs.ref.name + ";" + "\n"
            else:
                # if right hand side is a value
                if type(command.rhs.value) == str:
                    cpp_scheduler_template[14] += line_tabs + \
                                                  command.ref.name + " = \"" + command.rhs.value + "\";" + "\n"
                else:
                    cpp_scheduler_template[14] += line_tabs + \
                                                  command.ref.name + " = " + \
                                                  str(command.rhs.value) + ";" + "\n"


def operation_handler(command):

    """operation handler"""
    if command.lhs is not None:
        if command.lhs.__class__.__name__ == "Task_function":
            temp_command = ""
            if command.lhs.func.name == "next_deadline":
                temp_command += "getNextDeadline(\"" + command.lhs.task.name + "\",currentTime)"
            elif command.lhs.func.name == "wcet":
                temp_command += "segmentsByID.at(getJobID(\"" + command.lhs.task.name + "\")).getMaximalCost()"

            if command.rhs.__class__.__name__ == "Operation":
                return temp_command + " " + str(command.operator) + " " + operation_handler(command.rhs)

        elif command.lhs.__class__.__name__ == "Queue_function":
            temp_command = ""
            if command.lhs.func.name == "length":
                temp_command += "readyQueue.length()"
            elif command.lhs.func.name == "front().wcet":
                temp_command += "segmentsByID.find(readyQueue.front())->second.getMaximalCost()"
            elif command.lhs.func.name == "at":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.lhs.func.name == "back":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.lhs.func.name == "front":
                print("Error: cannot copy ready queue objects!")
                exit(1)

            if command.rhs.__class__.__name__ == "Operation":
                return temp_command + " " + str(command.operator) + " " + operation_handler(command.rhs)

        # elif command.lhs.__class__.__name__ == "Variable_value":

        elif command.lhs.ref.__class__.__name__ == "Variable":
            if command.rhs.__class__.__name__ == "Operation":
                return command.lhs.ref.name + " " + str(command.operator) + " " + operation_handler(command.rhs)
            elif command.rhs.ref.__class__.__name__ == "Variable":
                return command.lhs.ref.name + " " + str(command.operator) + " " + command.rhs.ref.name
            else:
                return command.lhs.ref.name + " " + command.operator + " " + str(command.rhs.value)
        elif command.lhs.__class__.__name__ == "Variable_value":
            if command.rhs.__class__.__name__ == "Operation":
                return str(command.lhs.ref.name) + " " + str(command.operator) + " " + operation_handler(command.rhs)
            elif command.rhs.ref.__class__.__name__ == "Variable":
                return str(command.lhs.ref.name) + " " + str(command.operator) + " " + command.rhs.ref.name
            else:
                return str(command.lhs.ref.name) + " " + command.operator + " " + str(command.rhs.value)
        else:
            if command.rhs.__class__.__name__ == "Operation":
                return str(command.lhs.value) + " " + command.operator + " " + operation_handler(command.rhs)
            elif command.rhs.__class__.__name__ == "Variable":
                return str(command.lhs.value) + " " + command.operator + " " + command.rhs.name
            else:
                return str(command.lhs.value) + " " + command.operator + " " + str(command.rhs.value)
    else:
        if command.rhs.__class__.__name__ == "Task_function":
            if command.rhs.func.name == "next_deadline":
                return "getNextDeadline(\"" + command.rhs.task.name + "\",currentTime)"
            elif command.rhs.func.name == "wcet":
                return "segmentsByID.at(getJobID(\"" + command.rhs.task.name + "\")).getMaximalCost()"

        elif command.rhs.__class__.__name__ == "Queue_function":
            if command.rhs.func.name == "length":
                return "readyQueue.length()"
            elif command.rhs.func.name == "front().wcet":
                return "segmentsByID.find(readyQueue.front())->second.getMaximalCost()"
            elif command.rhs.func.name == "at":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.rhs.func.name == "back":
                print("Error: cannot copy ready queue objects!")
                exit(1)
            elif command.rhs.func.name == "front":
                print("Error: cannot copy ready queue objects!")
                exit(1)


        elif command.rhs.__class__.__name__ == "Operation":
            return command.operator + " " + operation_handler(command.rhs)
        elif command.rhs.ref.__class__.__name__ == "Variable":
            return command.rhs.ref.name
        else:
            return str(command.rhs.value)


def condition_handler(command, line_tabs):
    """if statement handler"""

    # if we have right hand side
    if command.condition.rhs is not None:
        cpp_scheduler_template[14] += line_tabs + "if (" + \
                                      expression_handler(command.condition.lhs) + " " + \
                                      command.condition.specifier + " " + \
                                      expression_handler(command.condition.rhs) + ") {" + "\n"
    # if we don't have right hand side
    elif command.condition.rhs is None:
        cpp_scheduler_template[14] += line_tabs + "if (" + \
                                      expression_handler(command.condition.lhs) + ") {" + "\n"
    else:
        print("Error: invalid condition!")
        exit(1)
    # if the condition is true, then execute the commands
    scheduler_commands_handler(command.then_commands, line_tabs + "\t")
    # if the condition is false, then execute the else commands
    if len(command.else_commands) > 0:
        cpp_scheduler_template[14] += line_tabs + "} else {" + "\n"
        scheduler_commands_handler(command.else_commands, line_tabs + "\t")
    cpp_scheduler_template[14] += line_tabs + "}" + "\n"


def expression_handler(expression):
    """handles expressions and returns the corresponding C++ code"""
    if expression.__class__.__name__ == "Queue_function":
        # if left hand side is a queue function
        return queue_function_handler(expression)
    elif expression.__class__.__name__ == "Variable_value":
        if expression.ref is not None:
            # if right hand side is a variable
            return str(expression.ref.name)
        elif expression.value is not None:
            # if right hand side is a value
            return str(expression.value)
    elif expression.__class__.__name__ == "Current_time_function":
        return "currentTime"
    else:
        print(expression.__class__.__name__)
        print("Error: invalid expression!")
        exit(1)


def queue_function_handler(command):
    """handles queue functions and returns the corresponding C++ code"""
    if command.func.name == "length":
        return "readyQueue.length()"
    elif command.func.name == "empty":
        return "readyQueue.empty()"
    elif command.func.name == "front":
        return "readyQueue.front()"
    elif command.func.name == "back":
        return "readyQueue.back()"
    elif command.func.name == "at":
        if command.func.index.value is not None:
            return "readyQueue.at(" + str(command.func.index.value) + ")"
        elif command.func.index.ref is not None:
            return "readyQueue.at(" + command.func.index.ref.name + ")"
    else:
        print("Error: invalid queue function!")
        exit(1)


def check_rules(model):
    """check for errors in the user code"""
    dense = False
    event_all = False

    # 1. all events are not supported with dense time model
    # check if time model is dense
    if model.time_model != None and model.time_model.model == "dense":
        dense = True

    for e in model.events.event:
        if e == "all":
            event_all = True

    if dense and event_all:
        print("Error: dense time model does not support all events!")
        exit(1)

    # 2. more than one ready queue is not allowed
    # 3. more than one time stamp is not allowed
    # 4. ready queue and time stamp must have different names
    # 5. variable names must be unique
    queue_name = ""
    queue_size = 0

    time_name = ""
    time_size = 0

    var_names = []

    for c in model.scheduler.commands:
        if c.__class__.__name__ == "Ready_queue_declaration":
            queue_name = c.name
            queue_size += 1
        elif c.__class__.__name__ == "Current_time_declaration":
            time_name = c.name
            time_size += 1
        elif c.__class__.__name__ == "Variable_declaration":
            if c.var is not None:
                var_names.append(c.var.name)
    if queue_size > 1:
        print("Error: only one ready queue is allowed!")
        exit(1)

    if time_size > 1:
        print("Error: only one current time variable is allowed!")
        exit(1)

    if queue_name == time_name:
        print("Error: ready queue and current time variable must have different names!")
        exit(1)

    if queue_name in var_names:
        print("Error: ready queue and variable must have different names!")
        exit(1)
    elif time_name in var_names:
        print("Error: current time variable and variable must have different names!")
        exit(1)


def main():
    try:
        lang_mm = metamodel_from_file(
            'ReDo-MetaModel.tx', autokwd=True, debug=False)
    except Exception as e:
        # for the case that we run the script from project root directory
        try:
            lang_mm = metamodel_from_file(
                os.path.join('dsl','ReDo-MetaModel.tx'), autokwd=True, debug=False)
        except Exception as e:
            print("Error in meta-model:")
            print(e)
            exit(1)

    args = docopt(__doc__, version='0.5')
    input_file = Path(args['<file>'])
    # print(input_file)
    model = lang_mm.model_from_file(input_file)
    interpret(model)


if __name__ == '__main__':
    main()
