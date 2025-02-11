#ifndef DAG_HPP
#define DAG_HPP

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

struct node {

    node(unsigned long id, unsigned long timeStamp, std::string label)
            : id(id), timeStamp(timeStamp), label(label) {
    }

    unsigned long id;
    unsigned long timeStamp = 0;
    std::string label;
    std::vector<unsigned long> children;
    std::vector<unsigned long> parents;

};

struct edge {
    edge(unsigned long src, unsigned long dst, std::string label, std::string queue="")
            : fromID(src), toID(dst), label(label) {
        if (queue != "") {
            this->queue = "\\n" + queue;
        }
    }

    unsigned long fromID;
    unsigned long toID;
    std::string label;
    std::string queue;
};

class dag {
private:
    std::vector<std::shared_ptr<node>> nodes;
    std::vector<std::shared_ptr<edge>> edges;
    // a vector to keep ID of the leaves states
    std::vector<unsigned long> leaves;
    unsigned long numNodes = 0;

public:
    dag() = default;

    void addNode(long parentId, unsigned long timeStamp, std::string label, std::string edgeLabel, std::string edgeQueue="") {
		std::shared_ptr<node> n = std::make_shared<node>(numNodes, timeStamp, std::move(label));
        nodes.push_back(n);
        numNodes++;
        if (parentId != -1) {
            // find parent node
            auto x = std::find_if(nodes.begin(), nodes.end(), [parentId](std::shared_ptr<node> const &tempNode) {
                return tempNode->id == parentId;
            });
            if (x != nodes.end()) {
                std::shared_ptr<node> parent = *x;
                // add parent to child
                n->parents.push_back(parent->id);
                // add child to parent
                parent->children.push_back(n->id);
                // add edge
				std::shared_ptr<edge> e = std::make_shared<edge>(parent->id, n->id, std::move(edgeLabel), std::move(edgeQueue));
                edges.push_back(e);
            } else {
                log<LOG_CRITICAL>("Parent node in the DAG not found!");
                assert(false);
            }
        }

        // add new node id to leaves vector
        leaves.push_back(n->id);
        // remove its parent from the leaves vector
        if (parentId != -1) {
            leaves.erase(std::remove(leaves.begin(), leaves.end(), parentId), leaves.end());
        }


        // sort nodes by timestamp
        std::sort(nodes.begin(), nodes.end(),
                  [](const std::shared_ptr<node> &a, const std::shared_ptr<node> b) {
                      return a->timeStamp < b->timeStamp;
                  });

    }

    void addEdge(long source, long destination, std::string edgeLabel) {
        // find source node
        auto x = std::find_if(nodes.begin(), nodes.end(), [source](std::shared_ptr<node> const &tempNode) {
            return tempNode->id == source;
        });
        if (x != nodes.end()) {
            std::shared_ptr<node> sourceNode = *x;
            // find destination node
            auto y = std::find_if(nodes.begin(), nodes.end(), [destination](std::shared_ptr<node> const &tempNode) {
                return tempNode->id == destination;
            });
            if (y != nodes.end()) {
                std::shared_ptr<node> destinationNode = *y;
                // add destination to source
                sourceNode->children.push_back(destinationNode->id);
                // add source to destination
                destinationNode->parents.push_back(sourceNode->id);
                // add edge
				std::shared_ptr<edge> e = std::make_shared<edge>(sourceNode->id, destinationNode->id, std::move(edgeLabel));
                edges.push_back(e);
            } else {
                log<LOG_CRITICAL>("Destination node in the DAG not found!");
                assert(false);
            }
        } else {
            log<LOG_CRITICAL>("Source node in the DAG not found!");
            assert(false);
        }

        // remove the source node id from the leaves vector
        leaves.erase(std::remove(leaves.begin(), leaves.end(), source), leaves.end());
    }

	void freeMemory() {
		// remove the edges that are not connected to the leaves
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](std::shared_ptr<edge> const &tempEdge) {
			return std::find(leaves.begin(), leaves.end(), tempEdge->fromID) == leaves.end();
		}), edges.end());

		// if we don't need all the nodes, remove the none-leaf nodes from the node vector
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](std::shared_ptr<node> const &tempNode) {
			return !tempNode->children.empty();
		}), nodes.end());
	}

    void updateNodeLabel(unsigned long id, std::string label) {
        auto x = std::find_if(nodes.begin(), nodes.end(), [id](std::shared_ptr<node> const &tempNode) {
            return tempNode->id == id;
        });
        if (x != nodes.end()) {
            std::shared_ptr<node> n = *x;
            n->label = label;
        } else {
            log<LOG_CRITICAL>("Node in the DAG not found!");
            assert(false);
        }
    }

    // return the IDs of the leaves
    std::vector<unsigned long> getLeaves() {
        return leaves;
    }

    unsigned long getLeafStateWithSmallestTimeStamp() {
        for (const auto &n: nodes) {
            if (n->children.empty()) {
                return n->id;
            }
        }
        return -1;
    }

    unsigned long getStateWithSmallestTimeStamp() {
        return nodes[0]->id;
    }

    // check if we have edge we are looking for
    bool hasEdge(unsigned long fromID, const std::string& label, std::string queue="") {

        auto e = std::find_if(edges.begin(), edges.end(), [fromID, label](std::shared_ptr<edge> const &tempEdge) {
            return tempEdge->fromID == fromID && tempEdge->label == label;
        });

        if (e != edges.end()) {
            // add queue to the label
            (*e)->queue += " " + queue;
            return true;
        }

        return false;

    }

    // generate a dot file
    void generateDotFile(std::string filename) {
        std::ofstream file;
        file.open(filename);
        file << "digraph G {" << std::endl;
        file << "\trankdir=LR;" << std::endl;
        file << "\tnode [fontname=Ubuntu]" << std::endl;
        file << "\tedge [fontname=Ubuntu,color=Red,fontcolor=Red]" << std::endl;
        for (auto n: nodes) {
            file << "\t" << n->id << " [label=\"" << n->label << "\"];" << std::endl;
        }
        for (auto e: edges) {
            file << "\t" << e->fromID << " -> " << e->toID << " [label=\"" << e->label << (e->queue)  << "\"];" << std::endl;
        }
        file << "}" << std::endl;
        file.close();
    }

};

#endif //DAG_HPP
