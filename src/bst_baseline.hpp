// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// bst_baseline.hpp
// Árvore de busca binária COMUM (sem balanceamento). Existe apenas como linha
// de base nos experimentos de comparação (tests/compare.cpp): mostra o que
// acontece com a altura quando a ordem de chegada dos dados é desfavorável.
// Não faz parte do sistema e não é oferecida no menu.
//
// Implementação iterativa (sem recursão) porque, no pior caso, a altura é n.
// =============================================================================
#pragma once

#include <utility>
#include <vector>

#include "tree_interface.hpp"

class BaselineBST {
    struct Node {
        Key key;
        Node* l = nullptr;
        Node* r = nullptr;
        explicit Node(Key k) : key(k) {}
    };
    Node* root_ = nullptr;
    std::size_t n_ = 0;

public:
    BaselineBST() = default;
    BaselineBST(const BaselineBST&) = delete;
    BaselineBST& operator=(const BaselineBST&) = delete;

    ~BaselineBST() {
        std::vector<Node*> st;
        if (root_) st.push_back(root_);
        while (!st.empty()) {
            Node* x = st.back();
            st.pop_back();
            if (x->l) st.push_back(x->l);
            if (x->r) st.push_back(x->r);
            delete x;
        }
    }

    bool insert(Key k) {
        Node** p = &root_;
        while (*p) {
            if (k < (*p)->key)
                p = &(*p)->l;
            else if (k > (*p)->key)
                p = &(*p)->r;
            else
                return false;
        }
        *p = new Node(k);
        ++n_;
        return true;
    }

    std::size_t size() const { return n_; }

    // Altura em nós (mesma convenção das outras árvores).
    int height() const {
        int best = 0;
        std::vector<std::pair<const Node*, int>> st;
        if (root_) st.push_back({root_, 1});
        while (!st.empty()) {
            auto [x, d] = st.back();
            st.pop_back();
            if (d > best) best = d;
            if (x->l) st.push_back({x->l, d + 1});
            if (x->r) st.push_back({x->r, d + 1});
        }
        return best;
    }

    double avgDepth() const {
        if (!root_) return 0.0;
        long long sum = 0;
        std::vector<std::pair<const Node*, int>> st;
        st.push_back({root_, 1});
        while (!st.empty()) {
            auto [x, d] = st.back();
            st.pop_back();
            sum += d;
            if (x->l) st.push_back({x->l, d + 1});
            if (x->r) st.push_back({x->r, d + 1});
        }
        return static_cast<double>(sum) / static_cast<double>(n_);
    }
};
