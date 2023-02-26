#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

const std::unordered_set<int32_t> kEmptySet = {};

struct SaturationNode {
public:
    int32_t id;
    uint32_t saturation;
    uint32_t uncolored_neighborhood_degree;

    SaturationNode(uint32_t id, uint32_t saturation, uint32_t uncolored_neighborhood_degree):
            id(id),
            saturation(saturation),
            uncolored_neighborhood_degree(uncolored_neighborhood_degree) {
        // empty on purpose
    }

    SaturationNode(const SaturationNode& that) = default;
    SaturationNode& operator=(const SaturationNode& that) = default;

    ~SaturationNode() = default;
};

struct SaturationComparator {
    bool operator()(const SaturationNode& lhs, const SaturationNode& rhs) const {
        return std::tie(lhs.saturation, lhs.uncolored_neighborhood_degree, lhs.id) >
               std::tie(rhs.saturation, rhs.uncolored_neighborhood_degree, rhs.id);
    }
};


int32_t GenerateInRange(int32_t start, int32_t finish) {
    int32_t width = finish - start + 1;
    return static_cast<int32_t>(std::rand() % width + start);
}

class Graph {
private:
    size_t vertices_count_;
    std::vector<int32_t> vertices_;
    std::unordered_map<int32_t, std::unordered_set<int32_t>> adjacency_list_;

public:
    static std::unique_ptr<Graph> ReadGraphFile(const std::string& filename) {
        std::ifstream input_stream(filename);
        std::string line;

        std::unique_ptr<Graph> graph;

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
                graph = std::make_unique<Graph>(vertices);
            } else {
                int32_t from;
                int32_t to;
                line_input >> command >> from >> to;
                // Edges in DIMACS file can repeat.
                graph->AddEdge(from - 1, to - 1);
            }
        }

        return graph;
    }

    explicit Graph(size_t vertices_count):
            vertices_count_(vertices_count),
            vertices_(),
            adjacency_list_() {
        // empty on purpose
    }
    explicit Graph(const std::unordered_set<int32_t>& vertices):
            vertices_count_(vertices.size()),
            vertices_(vertices.begin(), vertices.end()),
            adjacency_list_() {
        for (const auto& v: vertices) {
            adjacency_list_[v] = {};
        }
    }
    Graph(const Graph& that) = default;
    Graph& operator=(const Graph& that) = default;

    [[nodiscard]] inline bool HasEdge(int32_t from, int32_t to) const {
        const auto& adjacent_to_from_vertexes = adjacency_list_.at(from);
        const auto& adjacent_to_to_vertexes = adjacency_list_.at(to);

        bool from_has_to = adjacent_to_from_vertexes.find(to) != adjacent_to_from_vertexes.end();
        bool to_has_from = adjacent_to_to_vertexes.find(from) != adjacent_to_to_vertexes.end();
        return from_has_to && to_has_from;
    }

    [[nodiscard]] inline std::vector<int32_t> GetVertices() {
        if (vertices_.size() != vertices_count_) {
            vertices_.clear();
            for (const auto& entry: adjacency_list_) {
                vertices_.emplace_back(entry.first);
            }
        }

        return vertices_;
    }

    [[nodiscard]] inline uint32_t GetDegree(int32_t vertex) const {
        if (adjacency_list_.find(vertex) == adjacency_list_.end()) {
            return 0;
        }

        return adjacency_list_.at(vertex).size();
    }

    [[nodiscard]] inline const std::unordered_set<int32_t>& GetAdjacentVertices(int32_t vertex) const {
        if (adjacency_list_.find(vertex) == adjacency_list_.end()) {
            return kEmptySet;
        }

        return adjacency_list_.at(vertex);
    }

    [[nodiscard]] inline size_t Size() const {
        return vertices_count_;
    }

    void AddEdge(int32_t from, int32_t to) {
        if (adjacency_list_.find(from) == adjacency_list_.end()) {
            adjacency_list_[from] = {};
        }
        adjacency_list_[from].insert(to);

        if (adjacency_list_.find(to) == adjacency_list_.end()) {
            adjacency_list_[to] = {};
        }
        adjacency_list_[to].insert(from);
    }

    void RemoveEdge(int32_t from, int32_t to) {
        adjacency_list_[from].erase(to);
        adjacency_list_[to].erase(from);
    }


    // [vertex, degree]
    [[nodiscard]] std::vector<std::tuple<int32_t, uint32_t>> SortVerticesByDegree(
            const std::unordered_set<int32_t>& vertices) const {
        std::vector<std::tuple<int32_t, uint32_t>> result;

        for (const auto& i: vertices) {
            uint32_t degree = 0;

            for (const auto& j: vertices) {
                if (i == j) {
                    continue;
                }

                if (HasEdge(i, j)) {
                    degree += 1;
                }
            }

            result.emplace_back(i, degree);
        }

        std::sort(result.begin(), result.end(), [](const std::tuple<int32_t, uint32_t>& one,
                                                   const std::pair<int32_t, uint32_t>& another) {
            return std::tie(std::get<1>(one), std::get<0>(one)) >
                std::tie(std::get<1>(another), std::get<0>(another));
        });

        return result;
    }


    [[nodiscard]] std::vector<std::tuple<int32_t, int32_t, int32_t>> SortVerticesByColor(
            const std::unordered_set<int32_t>& vertices) const {
        if (vertices.empty()) {
            return {};
        }

        Graph sub_graph(vertices);

        for (const auto& v: vertices) {
            const auto& neighbours = GetAdjacentVertices(v);

            for (const auto& n: neighbours) {
                if (vertices.find(n) != vertices.end()) {
                    sub_graph.AddEdge(v, n);
                }
            }
        }

        std::set<SaturationNode, SaturationComparator> queue;

        std::unordered_map<int32_t, uint32_t> vertices_degrees;
        std::unordered_map<int32_t, std::unordered_set<int32_t>> adjacent_colors;

        std::unordered_map<int32_t, int32_t> colors;

        for (const auto& i: sub_graph.GetVertices()) {
            const auto& adjacent_vertices = sub_graph.GetAdjacentVertices(i);

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
            for (const auto& neighbour: sub_graph.GetAdjacentVertices(node.id)) {
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

            for (const auto& neighbour: sub_graph.GetAdjacentVertices(node.id)) {
                if (colors[neighbour] != -1) {
                    continue;
                }

                SaturationNode old_neighbour_state(static_cast<uint32_t>(neighbour),
                                                   static_cast<uint32_t>(adjacent_colors[neighbour].size()),
                                                   vertices_degrees[neighbour]);

                if (adjacent_colors.find(neighbour) == adjacent_colors.end()) {
                    adjacent_colors[neighbour] = {};
                }

                adjacent_colors[neighbour].insert(current_color);
                vertices_degrees[neighbour] -= 1;
                queue.erase(old_neighbour_state);

                SaturationNode new_neighbour_state(static_cast<uint32_t>(neighbour),
                                                   static_cast<uint32_t>(adjacent_colors[neighbour].size()),
                                                   vertices_degrees[neighbour]);

                queue.insert(new_neighbour_state);
            }
        }

        std::vector<std::tuple<int32_t, int32_t, int32_t>> result;
        std::unordered_map<int32_t, std::unordered_map<int32_t, uint32_t>> colors_degree;

        for (const auto& v: sub_graph.GetVertices()) {
            const auto& neighbours = sub_graph.GetAdjacentVertices(v);

            if (colors_degree.find(v) == colors_degree.end()) {
                colors_degree[v] = {};
            }

            for (const auto& neighbour: neighbours) {
                const auto& neighbour_color = colors[neighbour];



                if (colors_degree[v].find(neighbour_color) == colors_degree[v].end()) {
                    colors_degree[v][neighbour_color] = 0;
                }

                colors_degree[v][neighbour_color] += 1;
            }
        }

        for (const auto& v: sub_graph.GetVertices()) {
            result.emplace_back(v, colors_degree[v].size(), sub_graph.GetDegree(v));
        }

        std::sort(result.begin(), result.end(), [](const std::tuple<int32_t, int32_t, int32_t>& one,
                                                   const std::tuple<int32_t, int32_t, int32_t>& another) {
            return std::tie(std::get<1>(one), std::get<2>(one), std::get<0>(one)) >
                std::tie(std::get<1>(another), std::get<2>(another), std::get<0>(another));
        });

        return result;
    }

    ~Graph() = default;
};

class Clique {
private:
    Graph* graph_;

    std::vector<int32_t> vertices_;
    std::unordered_set<int32_t> vertices_lookup_;

    std::unordered_set<int32_t> candidates_;

public:
    Clique(int32_t vertex,
           Graph* graph):
        graph_(graph),
        vertices_(),
        vertices_lookup_(),
        candidates_() {
        vertices_.push_back(vertex);
        vertices_lookup_.insert(vertex);

        for (int32_t v = 0; v < graph_->Size(); v++) {
            if (graph_->HasEdge(vertex, v)) {
                candidates_.insert(v);
            }
        }
    }

    Clique(const Clique& that) = default;
    Clique& operator=(const Clique& that) = default;

    [[nodiscard]] inline std::vector<int32_t> GetVertices() const {
        return vertices_;
    }

    [[nodiscard]] inline std::unordered_set<int32_t> GetCandidates() const {
        return candidates_;
    }

    [[nodiscard]] inline bool IsCandidate(int32_t vertex) const {
        return candidates_.find(vertex) != candidates_.end();
    }

    bool AddVertex(int32_t vertex) {
        if (vertices_lookup_.find(vertex) != vertices_lookup_.end()) {
            return false;
        }

        vertices_.push_back(vertex);
        vertices_lookup_.insert(vertex);

        std::vector<int32_t> should_be_removed;
        for (const auto& c: candidates_) {
            if (!graph_->HasEdge(c, vertex)) {
                should_be_removed.push_back(c);
            }
        }

        for (const auto& vr: should_be_removed) {
            candidates_.erase(vr);
        }

        return true;
    }

    bool RemoveVertex(int32_t vertex) {
        if (vertices_lookup_.find(vertex) == vertices_lookup_.end()) {
            return false;
        }

        vertices_.erase(
                std::remove(vertices_.begin(), vertices_.end(), vertex), vertices_.end());
        vertices_lookup_.erase(vertex);

        for (const auto& n: graph_->GetAdjacentVertices(vertex)) {
            bool adjacent_to_all_others = true;

            for (const auto& v: vertices_) {
                if (!graph_->HasEdge(n, v)) {
                    adjacent_to_all_others = false;
                    break;
                }
            }

            if (adjacent_to_all_others) {
                candidates_.insert(n);
            }
        }

        return true;
    }

    [[nodiscard]] bool Verify() const {
        std::unordered_set<int32_t> unique;

        for (const auto& node: vertices_) {
            unique.insert(node);
        }

        if (unique.size() != vertices_.size()) {
            std::cout << "Duplicated vertices in the clique\n";
            return false;
        }

        for (const auto& i: vertices_) {
            for (const auto& j: vertices_) {
                if (i != j && !graph_->HasEdge(i, j)) {
                    std::cout << "Returned subgraph is not a clique\n";
                    return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] inline bool IsNeededMoreCandidates() const {
        return !candidates_.empty();
    }

    [[nodiscard]] inline size_t CandidatesSize() const {
        return candidates_.size();
    }

    [[nodiscard]] inline size_t Size() const {
        return vertices_.size();
    }

    ~Clique() = default;
};

class MaxCliqueProblem {
private:
    std::unique_ptr<Graph> graph_;
    std::unique_ptr<Clique> best_clique_;

public:
    static MaxCliqueProblem FromFile(const std::string& file) {
        return MaxCliqueProblem(Graph::ReadGraphFile(file));
    }

    explicit MaxCliqueProblem(std::unique_ptr<Graph> graph):
        graph_(std::move(graph)),
        best_clique_() {
        // empty on purpose
    }

    MaxCliqueProblem(const MaxCliqueProblem& that) = delete;
    MaxCliqueProblem& operator=(const MaxCliqueProblem& that) = delete;
    MaxCliqueProblem(MaxCliqueProblem&& that) = default;
    MaxCliqueProblem& operator=(MaxCliqueProblem&& that) = default;

    [[nodiscard]] inline std::vector<int32_t> GetBestClique() const {
        return best_clique_->GetVertices();
    }

    [[nodiscard]] inline bool IsCliqueValid() const {
        return best_clique_ != nullptr && best_clique_->Verify();
    }

    void FindClique() {
        std::unordered_set<int32_t> vertices;
        for (int32_t v = 0; v < graph_->Size(); v++) {
            vertices.insert(v);
        }

        const auto& candidates = graph_->SortVerticesByColor(vertices);
        int32_t vertex = std::get<0>(candidates[0]);

        std::unique_ptr<Clique> init_clique = std::make_unique<Clique>(vertex, graph_.get());

        while (init_clique->IsNeededMoreCandidates()) {
            const auto& candidates = graph_->SortVerticesByColor(init_clique->GetCandidates());
            const auto& candidate = std::get<0>(candidates[0]);
            init_clique->AddVertex(candidate);
        }

        best_clique_ = std::move(init_clique);

        for (int32_t step = 0; step < 130; step++) {
            Clique* old_clique = best_clique_.get();
            std::unique_ptr<Clique> new_clique = std::make_unique<Clique>(*old_clique);

            const auto& clique_vertices = new_clique->GetVertices();

            std::unordered_set<int32_t> vertices_to_remove;
            vertices_to_remove.insert(GenerateInRange(0, static_cast<int32_t>(clique_vertices.size()) - 1));

            int32_t max_amount_to_remove = std::max(1, static_cast<int32_t>(new_clique->Size() * 0.7));

            // Starts from 1 as we already removed one.
            for (int32_t amount_to_remove = 1; amount_to_remove < max_amount_to_remove; amount_to_remove++) {
                int32_t new_vertex = GenerateInRange(0, static_cast<int32_t>(clique_vertices.size()) - 1);
                while (vertices_to_remove.find(new_vertex) != vertices_to_remove.end()) {
                    new_vertex = GenerateInRange(0, static_cast<int32_t>(clique_vertices.size()) - 1);
                }

                vertices_to_remove.insert(new_vertex);
            }

            for (const auto& vri: vertices_to_remove) {
                if (!new_clique->RemoveVertex(clique_vertices[vri])) {
                    throw std::runtime_error("Trying to remove vertex that is not in clique");
                }
            }

            while (new_clique->IsNeededMoreCandidates()) {
                const auto& candidates = graph_->SortVerticesByColor(new_clique->GetCandidates());
                // consider top 5 indexes.
                const auto& random_index = GenerateInRange(0, 1);
                int32_t index = std::min(random_index, static_cast<int32_t>(candidates.size()) - 1);

                const auto& candidate = std::get<0>(candidates[index]);
                new_clique->AddVertex(candidate);
            }

            if (new_clique->Size() > best_clique_->Size()) {
                best_clique_ = std::move(new_clique);
            }
        }
    }
};

double RoundTo(double value, double precision = 1.0) {
    return std::round(value / precision) * precision;
}

template<typename T>
std::string ConvertToString(
        const std::vector<T>& collection,
        const std::string& delimiter = " ") {
    std::ostringstream os;
    for (size_t i = 0; i < collection.size(); i++) {
        os << collection[i];
        if (i != collection.size() -1 ) {
            os << delimiter;
        }
    }
    return os.str();
}

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
                                       "sanr200_0.9.clq", "sanr400_0.7.clq"
    };

    std::ofstream fout("clique.csv");

    std::cout << std::setfill(' ') << std::setw(20) << "Instance"
              << std::setfill(' ') << std::setw(10) << "Clique"
              << std::setfill(' ') << std::setw(15) << "Time, sec"
              << std::endl;

    fout << "File; Clique; Time (sec); Clique vertices" << std::endl;
    for(const auto& file: files) {
        MaxCliqueProblem problem = MaxCliqueProblem::FromFile("data/" + file);
        clock_t start = clock();

        problem.FindClique();

        clock_t end = clock();
        clock_t ticks_diff = end - start;
        double seconds_diff = RoundTo(double(ticks_diff) / CLOCKS_PER_SEC, 0.001);

        if (!problem.IsCliqueValid()) {
            std::cout << "*** WARNING: incorrect clique ***\n";
            fout << "*** WARNING: incorrect clique ***\n";
            continue;
        }

        const auto& best_clique = problem.GetBestClique();

        fout << file << "; "
             << best_clique.size() << "; "
             << double(clock() - start) / CLOCKS_PER_SEC << "; "
             << ConvertToString(best_clique, ", ")
             << std::endl;

        std::cout << std::setfill(' ') << std::setw(20) << file
                  << std::setfill(' ') << std::setw(10) << best_clique.size()
                  << std::setfill(' ') << std::setw(15) << seconds_diff
                  << std::endl;
    }

    fout.close();
    return 0;
}
