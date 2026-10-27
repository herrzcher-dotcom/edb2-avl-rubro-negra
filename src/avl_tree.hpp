// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// avl_tree.hpp
// Árvore AVL (Adelson-Velsky e Landis), implementada do zero.
//
// Invariante: para todo nó, |altura(esq) - altura(dir)| <= 1.
//
// Estratégia de balanceamento: cada nó guarda a sua altura. Depois de cada
// inserção/remoção, voltando da recursão, recalculamos a altura de cada nó do
// caminho e, se o fator de balanceamento (fb = h(esq) - h(dir)) sair de
// [-1, +1], aplicamos uma das 4 rotações:
//
//   fb = +2 e fb(esq) >= 0  -> caso LL: rotação simples à direita
//   fb = +2 e fb(esq) <  0  -> caso LR: rotação à esquerda no filho + à direita
//   fb = -2 e fb(dir) <= 0  -> caso RR: rotação simples à esquerda
//   fb = -2 e fb(dir) >  0  -> caso RL: rotação à direita no filho + à esquerda
//
// (fb(filho) == 0 só ocorre na REMOÇÃO; nesse caso a rotação simples resolve.)
//
// Decisões de projeto (documentadas conforme o enunciado):
//   * Valor repetido na inserção  -> a árvore NÃO muda e insert() retorna false.
//   * Remover/buscar inexistente  -> a árvore NÃO muda e retorna false.
//   * Remoção de nó com 2 filhos  -> substitui pela chave do SUCESSOR em ordem
//     (menor da subárvore direita) e remove o sucessor.
// =============================================================================
#pragma once

#include <algorithm>
#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "tree_interface.hpp"

class AVLTree final : public ITree {
    struct Node {
        Key key;
        int h;  // altura do nó (folha = 1)
        Node* l;
        Node* r;
        explicit Node(Key k) : key(k), h(1), l(nullptr), r(nullptr) {}
    };

    Node* root_ = nullptr;
    std::size_t n_ = 0;

    // ------------------------------------------------------------ utilidades
    static int H(const Node* x) { return x ? x->h : 0; }
    static void update(Node* x) { x->h = 1 + std::max(H(x->l), H(x->r)); }
    static int bal(const Node* x) { return x ? H(x->l) - H(x->r) : 0; }

    static void destroy(Node* x) {
        if (!x) return;
        destroy(x->l);
        destroy(x->r);
        delete x;
    }

    // ------------------------------------------------------------- rotações
    //        y                x
    //       / \              / \.
    //      x   C    --->    A   y        rotação simples à direita em y
    //     / \                  / \.
    //    A   B                B   C
    Node* rotateRight(Node* y) {
        Node* x = y->l;
        y->l = x->r;
        x->r = y;
        update(y);  // y agora é filho: atualiza primeiro
        update(x);
        ++stats_.rotations;
        return x;
    }

    //      x                    y
    //     / \                  / \.
    //    A   y      --->      x   C       rotação simples à esquerda em x
    //       / \              / \.
    //      B   C            A   B
    Node* rotateLeft(Node* x) {
        Node* y = x->r;
        x->r = y->l;
        y->l = x;
        update(x);
        update(y);
        ++stats_.rotations;
        return y;
    }

    // Recalcula a altura de x e restaura a propriedade AVL, se necessário.
    // Retorna a nova raiz da subárvore.
    Node* rebalance(Node* x) {
        update(x);
        const int b = bal(x);
        if (b > 1) {  // pesado à esquerda
            if (bal(x->l) < 0) {
                ++stats_.avlLR;
                if (tracing())
                    note("AVL: nó " + std::to_string(x->key) + " com fb=+2 e filho esquerdo com fb<0 -> caso LR "
                         "(rotação à esquerda em " + std::to_string(x->l->key) + ", depois à direita em " +
                         std::to_string(x->key) + ")");
                x->l = rotateLeft(x->l);
                return rotateRight(x);
            }
            ++stats_.avlLL;
            if (tracing())
                note("AVL: nó " + std::to_string(x->key) + " com fb=+2 -> caso LL (rotação simples à direita em " +
                     std::to_string(x->key) + ")");
            return rotateRight(x);
        }
        if (b < -1) {  // pesado à direita
            if (bal(x->r) > 0) {
                ++stats_.avlRL;
                if (tracing())
                    note("AVL: nó " + std::to_string(x->key) + " com fb=-2 e filho direito com fb>0 -> caso RL "
                         "(rotação à direita em " + std::to_string(x->r->key) + ", depois à esquerda em " +
                         std::to_string(x->key) + ")");
                x->r = rotateRight(x->r);
                return rotateLeft(x);
            }
            ++stats_.avlRR;
            if (tracing())
                note("AVL: nó " + std::to_string(x->key) + " com fb=-2 -> caso RR (rotação simples à esquerda em " +
                     std::to_string(x->key) + ")");
            return rotateLeft(x);
        }
        return x;
    }

    // ------------------------------------------------------ inserção/remoção
    Node* insertRec(Node* x, Key k, bool& ok) {
        if (!x) {
            ok = true;
            return new Node(k);
        }
        if (k < x->key)
            x->l = insertRec(x->l, k, ok);
        else if (k > x->key)
            x->r = insertRec(x->r, k, ok);
        else {  // duplicado: nada muda
            ok = false;
            return x;
        }
        return ok ? rebalance(x) : x;
    }

    Node* removeRec(Node* x, Key k, bool& ok) {
        if (!x) {  // inexistente
            ok = false;
            return nullptr;
        }
        if (k < x->key)
            x->l = removeRec(x->l, k, ok);
        else if (k > x->key)
            x->r = removeRec(x->r, k, ok);
        else {
            ok = true;
            if (!x->l || !x->r) {  // 0 ou 1 filho: o filho (ou nulo) assume o lugar
                Node* child = x->l ? x->l : x->r;
                delete x;
                return child;
            }
            // 2 filhos: copia a chave do sucessor e remove o sucessor da direita
            Node* s = x->r;
            while (s->l) s = s->l;
            x->key = s->key;
            bool dummy = false;
            x->r = removeRec(x->r, s->key, dummy);
        }
        return ok ? rebalance(x) : x;
    }

    const Node* find(Key k) const {
        const Node* x = root_;
        while (x) {
            if (k < x->key)
                x = x->l;
            else if (k > x->key)
                x = x->r;
            else
                return x;
        }
        return nullptr;
    }

    // ------------------------------------------------------------ percursos
    static void collect(const Node* x, std::vector<Key>& out) {
        if (!x) return;
        collect(x->l, out);
        out.push_back(x->key);
        collect(x->r, out);
    }

    static void depthSum(const Node* x, int depth, long long& sum) {
        if (!x) return;
        sum += depth;
        depthSum(x->l, depth + 1, sum);
        depthSum(x->r, depth + 1, sum);
    }

    // ----------------------------------------------------------- impressão
    static std::string label(const Node* x) {
        const int b = bal(x);
        std::ostringstream s;
        s << x->key << " (h=" << x->h << ", fb=" << (b > 0 ? "+" : "") << b << ")";
        return s.str();
    }

    void printChildren(std::ostream& os, const Node* x, const std::string& prefix, int depth, int maxDepth) const {
        if (!x->l && !x->r) return;
        if (maxDepth > 0 && depth >= maxDepth) {
            os << prefix << "└── …\n";
            return;
        }
        std::vector<std::pair<char, const Node*>> ch;
        if (x->l) ch.push_back({'E', x->l});
        if (x->r) ch.push_back({'D', x->r});
        for (std::size_t i = 0; i < ch.size(); ++i) {
            const bool last = (i + 1 == ch.size());
            os << prefix << (last ? "└── " : "├── ") << ch[i].first << ": " << label(ch[i].second) << "\n";
            printChildren(os, ch[i].second, prefix + (last ? "    " : "│   "), depth + 1, maxDepth);
        }
    }

    int dotRec(const Node* x, std::ostream& o, int& id) const {
        const int me = id++;
        const int b = bal(x);
        o << "  n" << me << " [label=\"" << x->key << "\\nh=" << x->h << ", fb=" << (b > 0 ? "+" : "") << b << "\"];\n";
        const Node* ch[2] = {x->l, x->r};
        for (int i = 0; i < 2; ++i) {
            if (ch[i]) {
                const int c = dotRec(ch[i], o, id);
                o << "  n" << me << " -> n" << c << ";\n";
            } else if (x->l || x->r) {  // nó invisível para manter esquerda/direita
                const int g = id++;
                o << "  n" << g << " [style=invis, shape=point, width=0];\n";
                o << "  n" << me << " -> n" << g << " [style=invis];\n";
            }
        }
        return me;
    }

    // ----------------------------------------------------------- validação
    // Retorna a altura recalculada; preenche 'err' na primeira violação.
    int check(const Node* x, const Key* lo, const Key* hi, std::string& err, std::size_t& cnt) const {
        if (!x || !err.empty()) return 0;
        ++cnt;
        if ((lo && x->key <= *lo) || (hi && x->key >= *hi)) {
            err = "ordem de BST violada no nó " + std::to_string(x->key);
            return 0;
        }
        const int hl = check(x->l, lo, &x->key, err, cnt);
        const int hr = check(x->r, &x->key, hi, err, cnt);
        if (!err.empty()) return 0;
        const int h = 1 + std::max(hl, hr);
        if (h != x->h) {
            err = "altura armazenada incorreta no nó " + std::to_string(x->key);
            return 0;
        }
        if (std::abs(hl - hr) > 1) {
            err = "fator de balanceamento fora de [-1,+1] no nó " + std::to_string(x->key);
            return 0;
        }
        return h;
    }

public:
    AVLTree() = default;
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    ~AVLTree() override { destroy(root_); }

    std::string name() const override { return "AVL"; }

    bool insert(Key k) override {
        bool ok = false;
        root_ = insertRec(root_, k, ok);
        if (ok) {
            ++n_;
            ++stats_.insertsOk;
        } else {
            ++stats_.insertsDup;
        }
        return ok;
    }

    bool remove(Key k) override {
        bool ok = false;
        root_ = removeRec(root_, k, ok);
        if (ok) {
            --n_;
            ++stats_.removesOk;
        } else {
            ++stats_.removesMissing;
        }
        return ok;
    }

    bool contains(Key k) const override { return find(k) != nullptr; }

    std::vector<Key> inorder() const override {
        std::vector<Key> v;
        v.reserve(n_);
        collect(root_, v);
        return v;
    }

    std::size_t size() const override { return n_; }
    int height() const override { return H(root_); }

    double avgDepth() const override {
        if (!root_) return 0.0;
        long long sum = 0;
        depthSum(root_, 1, sum);
        return static_cast<double>(sum) / static_cast<double>(n_);
    }

    void print(std::ostream& os, bool /*ansi*/ = false, int maxDepth = 0) const override {
        if (!root_) {
            os << "(árvore vazia)\n";
            return;
        }
        os << label(root_) << "\n";
        printChildren(os, root_, "", 1, maxDepth);
    }

    std::string toDot() const override {
        std::ostringstream o;
        o << "digraph AVL {\n  graph [ordering=out];\n  node [shape=circle, fontname=\"Helvetica\"];\n";
        int id = 0;
        if (root_) dotRec(root_, o, id);
        o << "}\n";
        return o.str();
    }

    std::string validate() const override {
        std::string err;
        std::size_t cnt = 0;
        check(root_, nullptr, nullptr, err, cnt);
        if (err.empty() && cnt != n_) err = "contador de nós inconsistente com a estrutura";
        return err;
    }

    void clear() override {
        destroy(root_);
        root_ = nullptr;
        n_ = 0;
    }

    int childMask(Key k) const override {
        const Node* x = find(k);
        if (!x) return -1;
        return (x->l ? 1 : 0) | (x->r ? 2 : 0);
    }

    bool rootKey(Key& out) const override {
        if (!root_) return false;
        out = root_->key;
        return true;
    }
};
