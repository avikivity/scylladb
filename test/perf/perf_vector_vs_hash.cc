


#include "inet_address_vectors.hh"
#include <absl/container/flat_hash_set.h>
#include "utils/chunked_vector.hh"

#include <algorithm>
#include <random>

using abseil_inet_address_set = absl::flat_hash_set<gms::inet_address>;

void insert(abseil_inet_address_set& v, const gms::inet_address& x) {
    v.insert(x);
}

void insert(inet_address_vector_replica_set& v, const gms::inet_address& x) {
    v.push_back(x);
}

void erase(abseil_inet_address_set& v, const gms::inet_address& x) {
    v.erase(x);
}

void erase(inet_address_vector_replica_set& v, const gms::inet_address& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) {
        std::swap(*it, v.back());
        v.pop_back();
    }
}

struct topology {
    struct datacenter {
        std::vector<gms::inet_address> nodes;
    };
    std::vector<datacenter> datacenters;
    unsigned rf;
};

template <typename Container>
class test {
    utils::chunked_vector<Container> _elements;
    struct erasure {
        gms::inet_address erase_me;
        size_t element_index;
    };
    utils::chunked_vector<erasure> _erasures;
public:
    void prepare(topology topo, unsigned nr_elements) {
        auto re = std::default_random_engine();
        _elements.reserve(nr_elements);
        auto nr_replicas = topo.datacenters.size() * topo.rf;
        _erasures.reserve(nr_elements * nr_replicas);
        for (unsigned i = 0; i < nr_elements; ++i) {
            Container c;
            c.reserve(nr_replicas);
            for (auto& dc : topo.datacenters) {
                auto& nodes = dc.nodes;
                for (unsigned idx = 0; idx != topo.rf; ++idx) {
                    auto dist = std::uniform_int_distribution<size_t>(0, nodes.size() - 1);
                    std::swap(nodes[idx], nodes[dist(re)]);
                }
                for (unsigned idx = 0; idx != topo.rf; ++idx) {
                    auto& node = nodes[idx];
                    insert(c, node);
                    _erasures.push_back(erasure{node, i});
                }
            }
            _elements.push_back(std::move(c));
        }
        std::shuffle(_erasures.begin(), _erasures.end(), re);
    }
    void run() {
        for (auto& e : _erasures) {
            erase(_elements[e.element_index], e.erase_me);
        }
    }
};

topology make_topology(unsigned datacenters, unsigned nodes_per_dc, unsigned rf) {
    topology topo;
    topo.rf = rf;
    topo.datacenters.reserve(datacenters);
    for (unsigned i = 0; i != datacenters; ++i) {
        auto& dc = topo.datacenters.emplace_back();
        dc.nodes.reserve(nodes_per_dc);
        for (unsigned j = 0; j != nodes_per_dc; ++j) {
            unsigned ip = 0x10'00'00'00 | (i << 8) | j;
            dc.nodes.push_back(gms::inet_address(ip));
        }
    }
    return topo;
}

template <typename Container>
void run() {
    auto topo = make_topology(5, 10, 3);
    test<Container> t;
    t.prepare(topo, 10'000'000);
    t.run();
}

int main(int ac, char** av) {
    bool use_abseil = ac > 1 && std::string(av[1]) == "abseil";
    if (use_abseil) {
        run<abseil_inet_address_set>();
    } else {
        run<inet_address_vector_replica_set>();
    }
    return 0;
}
