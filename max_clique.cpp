#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace {

using Graph = std::unordered_map<int32_t, std::unordered_set<int32_t>>;

struct DegreeNode {
public:
    uint32_t id;
    uint32_t degree;

    DegreeNode(uint32_t id, uint32_t degree):
            id(id),
            degree(degree) {
        // empty on purpose
    }

    DegreeNode(const DegreeNode& that):
            id(that.id),
            degree(that.degree) {
        // empty on purpose
    }

    DegreeNode& operator=(const DegreeNode& that) {
        if (this != &that) {
            this->id = that.id;
            this->degree = that.degree;
        }

        return *this;
    }
};

struct SaturationNode {
public:
    uint32_t id;
    uint32_t saturation;
    uint32_t uncolored_neighborhood_degree;

    SaturationNode(uint32_t id, uint32_t saturation, uint32_t uncolored_neighborhood_degree):
            id(id),
            saturation(saturation),
            uncolored_neighborhood_degree(uncolored_neighborhood_degree) {
        // empty on purpose
    }

    SaturationNode(const SaturationNode& that):
            id(that.id),
            saturation(that.saturation),
            uncolored_neighborhood_degree(that.uncolored_neighborhood_degree) {
        // empty on purpose
    }

    SaturationNode& operator=(const SaturationNode& that) {
        if (this != &that) {
            this->id = that.id;
            this->saturation = that.saturation;
            this->uncolored_neighborhood_degree = that.uncolored_neighborhood_degree;
        }

        return *this;
    }
};

struct SaturationComparator {
    bool operator()(const SaturationNode& lhs, const SaturationNode& rhs) const {
        return std::tie(lhs.saturation, lhs.uncolored_neighborhood_degree, lhs.id) >
               std::tie(rhs.saturation, rhs.uncolored_neighborhood_degree, rhs.id);
    }
};

struct MaxDegreeComparator {
    bool operator()(const DegreeNode& lhs, const DegreeNode& rhs) const {
        return std::tie(lhs.degree, lhs.id) >
               std::tie(rhs.degree, rhs.id);
    }
};

struct MinDegreeComparator {
    bool operator()(const DegreeNode& lhs, const DegreeNode& rhs) const {
        return std::tie(lhs.degree, lhs.id) <
               std::tie(rhs.degree, rhs.id);
    }
};

/**
 * DSatur implementation of graph coloring.
 */
std::vector<int32_t> ColorGraph(Graph& graph) {
    std::set<SaturationNode, SaturationComparator> queue;

    std::vector<uint32_t> vertices_degrees(graph.size());
    std::vector<std::unordered_set<int32_t>> adjacent_colors(graph.size());

    std::vector<int32_t> colors(graph.size());

    for (auto i = 0; i < graph.size(); i++) {
        const auto& adjacent_vertices = graph[i];

        // let's reset all colors to kColorNoColor
        colors[i] = -1;
        vertices_degrees[i] = adjacent_vertices.size();

        queue.insert(SaturationNode(static_cast<uint32_t>(i),
                          static_cast<uint32_t>(adjacent_colors[i].size()),
                          vertices_degrees[i]));
    }

    while (!queue.empty()) {
        const auto queue_iterator = queue.begin();
        SaturationNode node = *queue_iterator;
        queue.erase(queue_iterator);

        int32_t current_color = -1;
        std::vector<bool> available_colors(colors.size(), true);
        for (const auto& neighbour: graph[node.id]) {
            int32_t color = colors[neighbour];
            if (color != -1) {
                available_colors[color] = false;
            }
        }
        for (size_t color = 0; color < available_colors.size(); color++) {
            if (available_colors[color]) {
                current_color = color;
                break;
            }
        }

        colors[node.id] = current_color;

        for (const auto& neighbour: graph[node.id]) {
            if (colors[neighbour] != -1) {
                continue;
            }

            SaturationNode old_neighbour_state(static_cast<uint32_t>(neighbour),
                                     static_cast<uint32_t>(adjacent_colors[neighbour].size()),
                                     vertices_degrees[neighbour]);

            adjacent_colors[neighbour].insert(current_color);
            vertices_degrees[neighbour] -= 1;
            queue.erase(old_neighbour_state);

            SaturationNode new_neighbour_state(static_cast<uint32_t>(neighbour),
                                     static_cast<uint32_t>(adjacent_colors[neighbour].size()),
                                     vertices_degrees[neighbour]);

            queue.insert(new_neighbour_state);
        }
    }

    return colors;
}

double RoundTo(double value, double precision = 1.0) {
    return std::round(value / precision) * precision;
}

Graph ReadGraphFile(const std::string& filename) {
    std::ifstream input_stream(filename);
    std::string line;

    Graph graph;

    uint32_t vertices = 0;
    uint32_t edges = 0;
    while (std::getline(input_stream, line)) {
        if (line[0] == 'c') {
            continue;
        }

        std::stringstream line_input(line);
        char command;

        if (line[0] == 'p') {
            std::string type;
            line_input >> command >> type >> vertices >> edges;
            for (uint32_t i = 0; i < vertices; i++) {
                graph[i] = {};
            }
        } else {
            int32_t from;
            int32_t to;
            line_input >> command >> from >> to;
            // Edges in DIMACS file can repeat.
            graph[from - 1].insert(to - 1);
            graph[to - 1].insert(from - 1);
        }
    }

    return graph;
}

class MaxCliqueProblem {
private:
    Graph graph_;

public:
    static MaxCliqueProblem FromFile(const std::string& file) {
        return MaxCliqueProblem(ReadGraphFile(file));
    }

    MaxCliqueProblem(const Graph& graph):
        graph_(graph) {
        // empty on purpose
    }

    MaxCliqueProblem(const MaxCliqueProblem& that):
            graph_(that.graph_) {
        // empty on purpose
    }

    MaxCliqueProblem& operator=(const MaxCliqueProblem& that) {
        if (this != &that) {
            this->graph_ = that.graph_;
        }

        return *this;
    }

    void RemoveSaturationNodeFromQueue(const SaturationNode& node,
                                       const std::vector<int32_t>& graph_coloring,
                                       std::set<SaturationNode, SaturationComparator>& queue,
                                       std::vector<uint32_t>& degrees,
                                       std::vector<std::unordered_map<int32_t, uint32_t>>& adjacent_colors) {
        if (queue.find(node) != queue.end()) {
            queue.erase(node);
        }

        const auto& node_color = graph_coloring[node.id];
        const auto& neighbours = graph_[node.id];

        // update neighbours
        for (const auto& neighbour: neighbours) {
            SaturationNode old_neighbour_state(static_cast<uint32_t>(neighbour) /* id */,
                                     static_cast<uint32_t>(adjacent_colors[neighbour].size()) /* saturation */,
                                     degrees[neighbour] /* uncolored_neighborhood_degree */ );

            if (queue.find(old_neighbour_state) == queue.end()) {
                continue;
            }

            queue.erase(old_neighbour_state);
            degrees[neighbour] -= 1;
            adjacent_colors[neighbour][node_color] -= 1;
            if (adjacent_colors[neighbour][node_color] == 0) {
                adjacent_colors[neighbour].erase(node_color);
            }

            SaturationNode new_neighbour_state(static_cast<uint32_t>(neighbour) /* id */,
                                     static_cast<uint32_t>(adjacent_colors[neighbour].size()) /* saturation */,
                                     degrees[neighbour] /* uncolored_neighborhood_degree */ );

            queue.insert(new_neighbour_state);
        }
    }

    void FindCliqueUsingColors(std::vector<int32_t>& clique) {
        std::vector<uint32_t> degrees(graph_.size());

        std::vector<int32_t> graph_coloring = ColorGraph(graph_);
        std::vector<std::unordered_map<int32_t, uint32_t>> adjacent_colors(graph_.size());

        for (auto node = 0; node < graph_.size(); node++) {
            const auto& neighbours = graph_[node];
            degrees[node] = neighbours.size();

            for (const auto& neighbour: neighbours) {
                const auto& neighbour_color = graph_coloring[neighbour];

                if (adjacent_colors[node].find(neighbour_color) == adjacent_colors[node].end()) {
                    adjacent_colors[node][neighbour_color] = 0;
                }

                adjacent_colors[node][neighbour_color] += 1;
            }
        }

        std::set<SaturationNode, SaturationComparator> queue;

        for (const auto& entry: graph_) {
            const auto& node = entry.first;

            queue.insert(SaturationNode(static_cast<uint32_t>(node) /* id */,
                              static_cast<uint32_t>(adjacent_colors[node].size()) /* saturation */,
                              degrees[node] /* uncolored_neighborhood_degree */ ));
        }

        while (!queue.empty()) {
            const auto queue_iterator = queue.begin();
            SaturationNode node = *queue_iterator;
            queue.erase(queue_iterator);

            clique.push_back(node.id);

            const auto& neighbours = graph_[node.id];

            RemoveSaturationNodeFromQueue(node, graph_coloring, queue, degrees, adjacent_colors);

            for (auto candidate = 0; candidate < graph_.size(); candidate++) {
                if (neighbours.find(candidate) != neighbours.end()) {
                    continue;
                }

                SaturationNode old_candidate_state(static_cast<uint32_t>(candidate) /* id */,
                                         static_cast<uint32_t>(adjacent_colors[candidate].size()) /* saturation */,
                                         degrees[candidate] /* uncolored_neighborhood_degree */ );

                if (queue.find(old_candidate_state) == queue.end()) {
                    continue;
                }

                RemoveSaturationNodeFromQueue(old_candidate_state, graph_coloring, queue, degrees, adjacent_colors);
            }
        }
    }

    template<class Comparator>
    void FindCliqueUsingDegree(std::vector<int32_t>& clique) {
        std::vector<uint32_t> degrees(graph_.size());

        for (auto node = 0; node < graph_.size(); node++) {
            const auto& neighbours = graph_[node];
            degrees[node] = neighbours.size();
        }

        std::set<DegreeNode, Comparator> queue;

        for (const auto& entry: graph_) {
            const auto& node = entry.first;

            queue.insert(DegreeNode(static_cast<uint32_t>(node),
                                        degrees[node]));
        }

        while (!queue.empty()) {
            const auto queue_iterator = queue.begin();
            DegreeNode node = *queue_iterator;
            queue.erase(queue_iterator);

            clique.push_back(node.id);

            const auto& neighbours = graph_[node.id];

            for (const auto& neighbour: neighbours) {
                DegreeNode old_neighbour_state(static_cast<uint32_t>(neighbour),
                                                   degrees[neighbour]);

                if (queue.find(old_neighbour_state) == queue.end()) {
                    continue;
                }

                queue.erase(old_neighbour_state);
                degrees[neighbour] -= 1;

                DegreeNode new_neighbour_state(static_cast<uint32_t>(neighbour),
                                               degrees[neighbour]);

                queue.insert(new_neighbour_state);
            }

            for (auto candidate = 0; candidate < graph_.size(); candidate++) {
                if (neighbours.find(candidate) != neighbours.end()) {
                    continue;
                }

                DegreeNode old_candidate_state(static_cast<uint32_t>(candidate),
                                               degrees[candidate]);

                if (queue.find(old_candidate_state) == queue.end()) {
                    continue;
                }

                queue.erase(old_candidate_state);

                const auto& neighbours = graph_[candidate];

                for (const auto& neighbour: neighbours) {
                    DegreeNode old_neighbour_state(static_cast<uint32_t>(neighbour),
                                                   degrees[neighbour]);

                    if (queue.find(old_neighbour_state) == queue.end()) {
                        continue;
                    }

                    queue.erase(old_neighbour_state);
                    degrees[neighbour] -= 1;

                    DegreeNode new_neighbour_state(static_cast<uint32_t>(neighbour),
                                                   degrees[neighbour]);

                    queue.insert(new_neighbour_state);
                }
            }
        }
    }

    void FindClique(std::vector<int32_t>& clique) {
        std::vector<int32_t> color_clique;
        FindCliqueUsingColors(color_clique);

        std::vector<int32_t> max_degree_clique;
        FindCliqueUsingDegree<MaxDegreeComparator>(max_degree_clique);

        if (color_clique.size() > max_degree_clique.size()) {
            clique.insert(clique.end(), color_clique.begin(), color_clique.end());
        } else {
            clique.insert(clique.end(), max_degree_clique.begin(), max_degree_clique.end());
        }
    }

    bool VerifyClique(const std::vector<int32_t>& clique) const {
        std::unordered_set<int32_t> unique;

        for (const auto& node: clique) {
            unique.insert(node);
        }

        if (unique.size() != clique.size()) {
            std::cout << "Duplicated vertices in the clique\n";
            return false;
        }

        for (const auto& i: clique) {
            for (const auto& j: clique) {
                if (i != j && (graph_.at(i).find(j) == graph_.at(i).end()
                    || graph_.at(j).find(i) == graph_.at(j).end())) {
                    std::cout << "Returned subgraph is not a clique\n";
                    return false;
                }
            }
        }
        return true;
    }
};

} // namespace

int main() {
    std::vector<std::string> files = { "brock200_1.clq", "brock200_2.clq", "brock200_3.clq", "brock200_4.clq",
                                       "brock400_1.clq", "brock400_2.clq", "brock400_3.clq", "brock400_4.clq",
                                       "C125.9.clq",
                                       "gen200_p0.9_44.clq", "gen200_p0.9_55.clq",
                                       "hamming8-4.clq",
                                       "johnson16-2-4.clq", "johnson8-2-4.clq",
                                       "keller4.clq",
                                       "MANN_a27.clq", "MANN_a9.clq",
                                       "p_hat1000-1.clq", "p_hat1000-2.clq",
                                       "p_hat1500-1.clq",
                                       "p_hat300-3.clq", "p_hat500-3.clq",
                                       "san1000.clq",
                                       "sanr200_0.9.clq", "sanr400_0.7.clq",
                                       "MANN_a45.clq"
    };

    std::ofstream fout("clique.csv");

    std::cout << std::setfill(' ') << std::setw(20) << "Instance"
              << std::setfill(' ') << std::setw(10) << "Clique"
              << std::setfill(' ') << std::setw(15) << "Time, sec"
              << std::endl;

    fout << "File; Clique; Time (sec)\n";
    for(const auto& file: files) {
        MaxCliqueProblem problem = MaxCliqueProblem::FromFile("data/" + file);
        clock_t start = clock();

        std::vector<int32_t> clique;
        problem.FindClique(clique);

        clock_t end = clock();
        clock_t ticks_diff = end - start;
        double seconds_diff = RoundTo(double(ticks_diff) / CLOCKS_PER_SEC, 0.001);

        if (!problem.VerifyClique(clique)) {
            std::cout << "*** WARNING: incorrect clique ***\n";
            fout << "*** WARNING: incorrect clique ***\n";
        }

        fout << file << "; " << clique.size() << "; " << double(clock() - start) / CLOCKS_PER_SEC << '\n';

        std::cout << std::setfill(' ') << std::setw(20) << file
                  << std::setfill(' ') << std::setw(10) << clique.size()
                  << std::setfill(' ') << std::setw(15) << seconds_diff
                  << std::endl;
    }

    fout.close();
    return 0;
}
