#ifndef DAG_HPP
#define DAG_HPP

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <cassert>

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
	// A map for nodes. The key is the node ID
	std::unordered_map<unsigned long, std::shared_ptr<node>> nodes;
    std::vector<std::shared_ptr<edge>> edges;
    // a vector to keep ID of the leaves states
    std::vector<unsigned long> leaves;
    unsigned long numNodes = 0;

public:
    dag() = default;

    void addNode(long parentId, unsigned long timeStamp, std::string label, std::string edgeLabel, std::string edgeQueue="") {
		std::shared_ptr<node> n = std::make_shared<node>(numNodes, timeStamp, std::move(label));
		nodes.insert({numNodes, n});
        numNodes++;
        if (parentId != -1) {
            // find parent node
			auto x = nodes.find(parentId);
            if (x != nodes.end()) {
                std::shared_ptr<node> parent = x->second;
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


    }

    void addEdge(long source, long destination, std::string edgeLabel) {
        // find source node
		auto x = nodes.find(source);
        if (x != nodes.end()) {
            std::shared_ptr<node> sourceNode = x->second;
            // find destination node
			auto y = nodes.find(destination);
            if (y != nodes.end()) {
                std::shared_ptr<node> destinationNode = y->second;
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
    // Remove the edges that are not connected to the leaves
    edges.erase(std::remove_if(edges.begin(), edges.end(), [&](const std::shared_ptr<edge> &tempEdge) {
        return !leaves.empty() && std::find(leaves.begin(), leaves.end(), tempEdge->fromID) == leaves.end();
    }), edges.end());

    // Remove the non-leaf nodes from the nodes map
    for (auto it = nodes.begin(); it != nodes.end();) {
        if (std::find(leaves.begin(), leaves.end(), it->first) == leaves.end()) {
            it = nodes.erase(it);
        } else {
            ++it;
        }
    }
}
    void updateNodeLabel(unsigned long id, std::string label) {
		auto x = nodes.find(id);
        if (x != nodes.end()) {
            std::shared_ptr<node> n = x->second;
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
		for (const auto &n: nodes) {
			file << "\t" << n.second->id << " [label=\"" << n.second->label << "\"];" << std::endl;
		}
        for (auto e: edges) {
            file << "\t" << e->fromID << " -> " << e->toID << " [label=\"" << e->label << (e->queue)  << "\"];" << std::endl;
        }
        file << "}" << std::endl;
        file.close();
    }

};

#endif //DAG_HPP
