#ifndef JNGEN_INCLUDE_GENERIC_GRAPH_INL_H
#error File "generic_graph_inl.h" must not be included directly.
#include "../generic_graph.h" // for completion emgine
#endif

namespace jngen {

void GenericGraph::setVertexWeights(const WeightArray& weights) {
    ensure(
        static_cast<int>(weights.size()) == n(),
        "The argument of setVertexWeights must have exactly n elements");
    vertexWeights_.resize(n());
    for (int i = 0; i < n(); ++i) {
        vertexWeights_[i] = weights[vertexByLabel(i)];
    }
}

void GenericGraph::setVertexWeight(int v, const Weight& weight) {
    ensure(v < n(), "setVertexWeight");
    v = vertexByLabel(v);

    vertexWeights_.extend(v + 1);
    vertexWeights_[v] = weight;
}

void GenericGraph::setEdgeWeights(const WeightArray& weights) {
    ensure(
        static_cast<int>(weights.size()) == m(),
        "The argument of setEdgeWeights must have exactly m elements");
    edgeWeights_ = weights;
}

void GenericGraph::setEdgeWeight(size_t index, const Weight& weight) {
    ensure(static_cast<int>(index) < m(), "setEdgeWeight");
    edgeWeights_.extend(index + 1);
    edgeWeights_[index] = weight;
}

Weight GenericGraph::vertexWeight(int v) const {
    ensure(v < n(), "vertexWeight");
    size_t index = vertexByLabel(v);
    if (index >= vertexWeights_.size()) {
        return Weight{};
    }
    return vertexWeights_[index];
}

Weight GenericGraph::edgeWeight(size_t index) const {
    ensure(static_cast<int>(index) < m(), "edgeWeight");
    if (index >= edgeWeights_.size()) {
        return Weight{};
    }
    return edgeWeights_[index];
}

Array GenericGraph::edges(int v) const {

    ensure(v < n(), "Graph::edges(v)");
    v = vertexByLabel(v);

    Array result = internalEdges(v);
    for (auto& x: result) {
        x = vertexLabel(x);
    }

    return result;
}

Arrayp GenericGraph::edges() const {
    auto edges = edges_;
    for (auto& e: edges) {
        e.first = vertexLabel(e.first);
        e.second = vertexLabel(e.second);
    }
    return edges;
}

WeightArray GenericGraph::prepareWeightArray(WeightArray a, int requiredSize) {
    ENSURE(a.hasNonEmpty(), "Attempt to print empty weight array");

    a.extend(requiredSize);
    int type = a.anyType();
    for (auto& x: a) {
        if (x.empty()) {
            x.setType(type);
        }
    }

    return a;
}

void GenericGraph::doShuffle() {
    // this if is to be removed after all checks pass
    if (vertexLabel_.size() < static_cast<size_t>(n())) {
        ENSURE(false, "GenericGraph::doShuffle");
        vertexLabel_ = Array::id(n());
    }

    vertexLabel_.shuffle();
    vertexByLabel_ = vertexLabel_.inverse();

    if (!directed_) {
        for (auto& edge: edges_) {
            if (rnd.next(2)) {
                std::swap(edge.first, edge.second);
            }
        }
    }

    permuteEdges(Array::id(numEdges_).shuffled());
}

void GenericGraph::extend(size_t size) {
    checkLargeParameter(size);
    size_t oldSize = n();
    if (size > oldSize) {
        adjList_.resize(size);
        vertexLabel_ += Array::id(size - oldSize, oldSize);
        vertexByLabel_ += Array::id(size - oldSize, oldSize);
        dsu_.extend(size);
    }
}

Array GenericGraph::internalEdges(int v) const {
    Array result;
    std::transform(
        adjList_[v].begin(),
        adjList_[v].end(),
        std::back_inserter(result),
        [this, v](int x) { return edgeOtherEnd(v, x); }
    );
    return result;
}

void GenericGraph::addEdgeUnsafe(int u, int v) {
    int id = numEdges_++;
    edges_.emplace_back(u, v);

    ENSURE(u < n() && v < n(), "GenericGraph::addEdgeUnsafe");

    adjList_[u].push_back(id);
    if (!directed_ && u != v) {
        adjList_[v].push_back(id);
    }
}

int GenericGraph::edgeOtherEnd(int v, int edgeId) const {
    ENSURE(edgeId < numEdges_);
    const auto& edge = edges_[edgeId];
    if (edge.first == v) {
        return edge.second;
    }
    ENSURE(!directed_);
    ENSURE(edge.second == v);
    return edge.first;
}

void GenericGraph::permuteEdges(const Array& order) {
    ENSURE(static_cast<int>(order.size()) == m(), "GenericGraph::permuteEdges");

    edges_ = edges_.subseq(order);

    auto newByOld = order.inverse();
    for (int v = 0; v < n(); ++v) {
        for (auto& x: adjList_[v]) {
            x = newByOld[x];
        }
    }

    if (edgeWeights_.hasNonEmpty()) {
        edgeWeights_.extend(m());
        edgeWeights_ = edgeWeights_.subseq(order);
    }
}

void GenericGraph::addEdge(int u, int v, const Weight& w) {
    extend(std::max(u, v) + 1);

    u = vertexByLabel(u);
    v = vertexByLabel(v);

    dsu_.unite(u, v);
    addEdgeUnsafe(u, v);

    if (!w.empty()) {
        setEdgeWeight(m() - 1, w);
    }
}

void GenericGraph::doPrintEdges(
    std::ostream& out, const OutputModifier& mod) const
{
    if (mod.printN) {
        out << n();
        if (mod.printM) {
            out << " " << m();
        }
        out << "\n";
    } else if (mod.printM) {
        out << m() << "\n";
    }

    if (vertexWeights_.hasNonEmpty()) {
        auto vertexWeights = prepareWeightArray(vertexWeights_, n());
        for (int i = 0; i < n(); ++i) {
            if (i > 0) {
                out << " ";
            }
            JNGEN_PRINT_NO_MOD(vertexWeights[vertexByLabel(i)]);
        }
        out << "\n";
    }

    auto t(mod);
    {
        auto mod(t);

        Arrayp edges = this->edges();
        mod.printN = false;
        if (edgeWeights_.hasNonEmpty()) {
            auto edgeWeights = prepareWeightArray(edgeWeights_, m());
            for (int i = 0; i < m(); ++i) {
                if (i > 0) {
                    out << "\n";
                }
                JNGEN_PRINT(edges[i]);
                out << " ";
                JNGEN_PRINT_NO_MOD(edgeWeights[i]);
            }
        } else {
            JNGEN_PRINT(edges);
        }
    }
}

bool GenericGraph::operator==(const GenericGraph& other) const {
    return compareTo(other) == 0;
}

bool GenericGraph::operator!=(const GenericGraph& other) const {
    return compareTo(other) != 0;
}

bool GenericGraph::operator<(const GenericGraph& other) const {
    return compareTo(other) == -1;
}

bool GenericGraph::operator>(const GenericGraph& other) const {
    return compareTo(other) == 1;
}

bool GenericGraph::operator<=(const GenericGraph& other) const {
    return compareTo(other) != 1;
}

bool GenericGraph::operator>=(const GenericGraph& other) const {
    return compareTo(other) != -1;
}

void GenericGraph::doNormalizeEdges() {
    ENSURE(
        vertexLabel_ == Array::id(n()),
        "Can call normalizeEdges() only on newly created graph");

    if (!directed_) {
        for (auto& edge: edges_) {
            if (edge.first > edge.second) {
                std::swap(edge.first, edge.second);
            }
        }
    }

    auto order = Array::id(numEdges_).sorted(
        [this](int i, int j) {
            return edges_[i] < edges_[j];
        });

    permuteEdges(order);
}


int GenericGraph::compareTo(const GenericGraph& other) const {
    if (n() != other.n()) {
        return n() < other.n() ? -1 : 1;
    }
    for (int i = 0; i < n(); ++i) {
        auto e1 = edges(i).sorted();
        auto e2 = other.edges(i).sorted();
        if (e1 != e2) {
            return e1 < e2 ? -1 : 1;
        }
    }
    return 0;
}

} // namespace jngen
