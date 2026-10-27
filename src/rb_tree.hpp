// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// rb_tree.hpp
// Árvore Rubro-Negra (Red-Black Tree), implementada do zero seguindo o
// algoritmo do CLRS (Cormen et al.), com nó sentinela "nil".
//
// Propriedades (todas verificadas por validate()):
//   1. Todo nó é rubro ou negro.
//   2. A raiz é negra.
//   3. Toda folha (nil) é negra.
//   4. Filhos de um nó rubro são negros (não há dois rubros seguidos).
//   5. Todo caminho de um nó até as folhas descendentes tem o mesmo número de
//      nós negros (altura negra).
//   Consequência: altura <= 2*log2(n+1) (medida em arestas).
//
// Estratégia de balanceamento:
//   INSERÇÃO: o novo nó entra RUBRO (não altera a altura negra). Se o pai também
//   é rubro (violação da propriedade 4), olhamos o TIO:
//     caso 1 (tio rubro)         -> recolore pai e tio de negro e o avô de rubro;
//                                   o problema sobe dois níveis;
//     caso 2 (tio negro, "joelho")-> rotação para virar o caso 3;
//     caso 3 (tio negro, "reta")  -> recolore e faz 1 rotação; termina.
//   REMOÇÃO: se o nó realmente removido era negro, surge um "duplo negro" (x),
//   tratado em 4 casos (irmão rubro / irmão negro com sobrinhos negros / sobrinho
//   distante rubro / sobrinho próximo rubro), com no máximo 3 rotações.
//
// Decisões de projeto:
//   * Valor repetido na inserção  -> a árvore NÃO muda e insert() retorna false.
//   * Remover/buscar inexistente  -> a árvore NÃO muda e retorna false.
//   * Remoção de nó com 2 filhos  -> usa o SUCESSOR em ordem (como no CLRS).
//   * Contagem: cada rotação simples = 1; "recolorações" = quantas vezes a cor de
//     um nó real mudou de fato (colorir com a cor que já tinha não conta).
// =============================================================================
#pragma once

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "tree_interface.hpp"

class RBTree final : public ITree {
    enum Color : unsigned char { RED, BLACK };

    struct Node {
        Key key;
        Color c;
        Node* l;
        Node* r;
        Node* p;
    };

    Node* nil_;   // sentinela única (negra) que representa todas as folhas
    Node* root_;
    std::size_t n_ = 0;

    // ------------------------------------------------------------ utilidades
    void destroy(Node* x) {
        if (x == nil_) return;
        destroy(x->l);
        destroy(x->r);
        delete x;
    }

    // Troca a cor contando apenas mudanças reais.
    void paint(Node* x, Color c) {
        if (x == nil_ || x->c == c) return;
        x->c = c;
        ++stats_.recolorings;
    }

    Node* find(Key k) const {
        Node* x = root_;
        while (x != nil_) {
            if (k < x->key)
                x = x->l;
            else if (k > x->key)
                x = x->r;
            else
                return x;
        }
        return nil_;
    }

    Node* minimum(Node* x) const {
        while (x->l != nil_) x = x->l;
        return x;
    }

    // ------------------------------------------------------------- rotações
    void rotateLeft(Node* x) {
        Node* y = x->r;
        x->r = y->l;
        if (y->l != nil_) y->l->p = x;
        y->p = x->p;
        if (x->p == nil_)
            root_ = y;
        else if (x == x->p->l)
            x->p->l = y;
        else
            x->p->r = y;
        y->l = x;
        x->p = y;
        ++stats_.rotations;
    }

    void rotateRight(Node* y) {
        Node* x = y->l;
        y->l = x->r;
        if (x->r != nil_) x->r->p = y;
        x->p = y->p;
        if (y->p == nil_)
            root_ = x;
        else if (y == y->p->r)
            y->p->r = x;
        else
            y->p->l = x;
        x->r = y;
        y->p = x;
        ++stats_.rotations;
    }

    // ------------------------------------------------------------- inserção
    void insertFix(Node* z) {
        while (z->p->c == RED) {  // pai rubro => avô existe e é negro
            Node* g = z->p->p;
            if (z->p == g->l) {
                Node* u = g->r;  // tio
                if (u->c == RED) {  // caso 1: tio rubro -> só recolore
                    if (tracing())
                        note("RB: nó " + std::to_string(z->key) + " (rubro) com pai " + std::to_string(z->p->key) +
                             " rubro e tio " + std::to_string(u->key) + " rubro -> caso 1: recolore pai e tio (negro) e avô " +
                             std::to_string(g->key) + " (rubro)");
                    paint(z->p, BLACK);
                    paint(u, BLACK);
                    paint(g, RED);
                    z = g;
                } else {
                    if (z == z->p->r) {  // caso 2: "joelho" -> rotação para cair no caso 3
                        if (tracing())
                            note("RB: tio negro e nó " + std::to_string(z->key) + " é filho direito -> caso 2: rotação à esquerda em " +
                                 std::to_string(z->p->key));
                        z = z->p;
                        rotateLeft(z);
                    }
                    if (tracing())
                        note("RB: tio negro e nó em linha reta -> caso 3: pai " + std::to_string(z->p->key) +
                             " fica negro, avô " + std::to_string(g->key) + " fica rubro e rotação à direita no avô");
                    paint(z->p, BLACK);
                    paint(g, RED);
                    rotateRight(g);
                }
            } else {  // simétrico
                Node* u = g->l;
                if (u->c == RED) {
                    if (tracing())
                        note("RB: nó " + std::to_string(z->key) + " (rubro) com pai " + std::to_string(z->p->key) +
                             " rubro e tio " + std::to_string(u->key) + " rubro -> caso 1: recolore pai e tio (negro) e avô " +
                             std::to_string(g->key) + " (rubro)");
                    paint(z->p, BLACK);
                    paint(u, BLACK);
                    paint(g, RED);
                    z = g;
                } else {
                    if (z == z->p->l) {
                        if (tracing())
                            note("RB: tio negro e nó " + std::to_string(z->key) + " é filho esquerdo -> caso 2: rotação à direita em " +
                                 std::to_string(z->p->key));
                        z = z->p;
                        rotateRight(z);
                    }
                    if (tracing())
                        note("RB: tio negro e nó em linha reta -> caso 3: pai " + std::to_string(z->p->key) +
                             " fica negro, avô " + std::to_string(g->key) + " fica rubro e rotação à esquerda no avô");
                    paint(z->p, BLACK);
                    paint(g, RED);
                    rotateLeft(g);
                }
            }
        }
        paint(root_, BLACK);  // propriedade 2
    }

    // -------------------------------------------------------------- remoção
    void transplant(Node* u, Node* v) {
        if (u->p == nil_)
            root_ = v;
        else if (u == u->p->l)
            u->p->l = v;
        else
            u->p->r = v;
        v->p = u->p;  // vale mesmo se v == nil_ (CLRS usa isso em deleteFix)
    }

    void deleteFix(Node* x) {
        while (x != root_ && x->c == BLACK) {
            if (x == x->p->l) {
                Node* w = x->p->r;  // irmão (nunca é nil num duplo-negro)
                if (w->c == RED) {  // caso 1: irmão rubro
                    if (tracing())
                        note("RB: duplo-negro; irmão " + std::to_string(w->key) + " rubro -> caso 1: recolore e rotação à esquerda em " +
                             std::to_string(x->p->key));
                    paint(w, BLACK);
                    paint(x->p, RED);
                    rotateLeft(x->p);
                    w = x->p->r;
                }
                if (w->l->c == BLACK && w->r->c == BLACK) {  // caso 2: sobrinhos negros
                    if (tracing())
                        note("RB: irmão " + std::to_string(w->key) + " negro com sobrinhos negros -> caso 2: irmão vira rubro; problema sobe");
                    paint(w, RED);
                    x = x->p;
                } else {
                    if (w->r->c == BLACK) {  // caso 3: sobrinho distante negro
                        if (tracing())
                            note("RB: sobrinho distante negro -> caso 3: recolore e rotação à direita em " + std::to_string(w->key));
                        paint(w->l, BLACK);
                        paint(w, RED);
                        rotateRight(w);
                        w = x->p->r;
                    }
                    if (tracing())
                        note("RB: sobrinho distante rubro -> caso 4: irmão herda a cor do pai, rotação à esquerda em " +
                             std::to_string(x->p->key) + "; termina");
                    paint(w, x->p->c);  // caso 4
                    paint(x->p, BLACK);
                    paint(w->r, BLACK);
                    rotateLeft(x->p);
                    x = root_;
                }
            } else {  // simétrico
                Node* w = x->p->l;
                if (w->c == RED) {
                    if (tracing())
                        note("RB: duplo-negro; irmão " + std::to_string(w->key) + " rubro -> caso 1: recolore e rotação à direita em " +
                             std::to_string(x->p->key));
                    paint(w, BLACK);
                    paint(x->p, RED);
                    rotateRight(x->p);
                    w = x->p->l;
                }
                if (w->r->c == BLACK && w->l->c == BLACK) {
                    if (tracing())
                        note("RB: irmão " + std::to_string(w->key) + " negro com sobrinhos negros -> caso 2: irmão vira rubro; problema sobe");
                    paint(w, RED);
                    x = x->p;
                } else {
                    if (w->l->c == BLACK) {
                        if (tracing())
                            note("RB: sobrinho distante negro -> caso 3: recolore e rotação à esquerda em " + std::to_string(w->key));
                        paint(w->r, BLACK);
                        paint(w, RED);
                        rotateLeft(w);
                        w = x->p->l;
                    }
                    if (tracing())
                        note("RB: sobrinho distante rubro -> caso 4: irmão herda a cor do pai, rotação à direita em " +
                             std::to_string(x->p->key) + "; termina");
                    paint(w, x->p->c);
                    paint(x->p, BLACK);
                    paint(w->l, BLACK);
                    rotateRight(x->p);
                    x = root_;
                }
            }
        }
        paint(x, BLACK);
    }

    // ------------------------------------------------------------ percursos
    void collect(const Node* x, std::vector<Key>& out) const {
        if (x == nil_) return;
        collect(x->l, out);
        out.push_back(x->key);
        collect(x->r, out);
    }

    int heightOf(const Node* x) const {
        if (x == nil_) return 0;
        const int a = heightOf(x->l), b = heightOf(x->r);
        return 1 + (a > b ? a : b);
    }

    void depthSum(const Node* x, int depth, long long& sum) const {
        if (x == nil_) return;
        sum += depth;
        depthSum(x->l, depth + 1, sum);
        depthSum(x->r, depth + 1, sum);
    }

    // ----------------------------------------------------------- impressão
    static std::string label(const Node* x, bool ansi) {
        std::ostringstream s;
        const bool red = (x->c == RED);
        if (ansi && red) s << "\033[31m";
        s << x->key << " (" << (red ? 'R' : 'N') << ")";
        if (ansi && red) s << "\033[0m";
        return s.str();
    }

    void printChildren(std::ostream& os, const Node* x, const std::string& prefix, int depth, int maxDepth, bool ansi) const {
        if (x->l == nil_ && x->r == nil_) return;
        if (maxDepth > 0 && depth >= maxDepth) {
            os << prefix << "└── …\n";
            return;
        }
        std::vector<std::pair<char, const Node*>> ch;
        if (x->l != nil_) ch.push_back({'E', x->l});
        if (x->r != nil_) ch.push_back({'D', x->r});
        for (std::size_t i = 0; i < ch.size(); ++i) {
            const bool last = (i + 1 == ch.size());
            os << prefix << (last ? "└── " : "├── ") << ch[i].first << ": " << label(ch[i].second, ansi) << "\n";
            printChildren(os, ch[i].second, prefix + (last ? "    " : "│   "), depth + 1, maxDepth, ansi);
        }
    }

    int dotRec(const Node* x, std::ostream& o, int& id) const {
        const int me = id++;
        if (x->c == RED)
            o << "  n" << me << " [label=\"" << x->key << "\", style=filled, fillcolor=\"#d62728\", fontcolor=white];\n";
        else
            o << "  n" << me << " [label=\"" << x->key << "\", style=filled, fillcolor=\"#1a1a1a\", fontcolor=white];\n";
        const Node* ch[2] = {x->l, x->r};
        for (int i = 0; i < 2; ++i) {
            if (ch[i] != nil_) {
                const int c = dotRec(ch[i], o, id);
                o << "  n" << me << " -> n" << c << ";\n";
            } else if (x->l != nil_ || x->r != nil_) {
                const int g = id++;
                o << "  n" << g << " [style=invis, shape=point, width=0];\n";
                o << "  n" << me << " -> n" << g << " [style=invis];\n";
            }
        }
        return me;
    }

    // ----------------------------------------------------------- validação
    // Retorna a altura negra (nós negros no caminho até a folha, sem contar nil).
    int check(const Node* x, const Key* lo, const Key* hi, std::string& err, std::size_t& cnt) const {
        if (x == nil_ || !err.empty()) return 0;
        ++cnt;
        if ((lo && x->key <= *lo) || (hi && x->key >= *hi)) {
            err = "ordem de BST violada no nó " + std::to_string(x->key);
            return 0;
        }
        if (x->c == RED && (x->l->c == RED || x->r->c == RED)) {
            err = "propriedade 4 violada: nó rubro " + std::to_string(x->key) + " com filho rubro";
            return 0;
        }
        if ((x->l != nil_ && x->l->p != x) || (x->r != nil_ && x->r->p != x)) {
            err = "ponteiro para o pai inconsistente abaixo do nó " + std::to_string(x->key);
            return 0;
        }
        const int bl = check(x->l, lo, &x->key, err, cnt);
        const int br = check(x->r, &x->key, hi, err, cnt);
        if (!err.empty()) return 0;
        if (bl != br) {
            err = "propriedade 5 violada: alturas negras diferentes no nó " + std::to_string(x->key);
            return 0;
        }
        return bl + (x->c == BLACK ? 1 : 0);
    }

public:
    RBTree() {
        nil_ = new Node{0, BLACK, nullptr, nullptr, nullptr};
        nil_->l = nil_->r = nil_->p = nil_;
        root_ = nil_;
    }
    RBTree(const RBTree&) = delete;
    RBTree& operator=(const RBTree&) = delete;
    ~RBTree() override {
        destroy(root_);
        delete nil_;
    }

    std::string name() const override { return "Rubro-Negra"; }

    bool insert(Key k) override {
        Node* y = nil_;
        Node* x = root_;
        while (x != nil_) {
            y = x;
            if (k < x->key)
                x = x->l;
            else if (k > x->key)
                x = x->r;
            else {  // duplicado: nada muda
                ++stats_.insertsDup;
                return false;
            }
        }
        Node* z = new Node{k, RED, nil_, nil_, y};
        if (y == nil_)
            root_ = z;
        else if (k < y->key)
            y->l = z;
        else
            y->r = z;
        ++n_;
        ++stats_.insertsOk;
        insertFix(z);
        return true;
    }

    bool remove(Key k) override {
        Node* z = find(k);
        if (z == nil_) {  // inexistente
            ++stats_.removesMissing;
            return false;
        }
        Node* y = z;
        Color yOrig = y->c;
        Node* x;
        if (z->l == nil_) {
            x = z->r;
            transplant(z, z->r);
        } else if (z->r == nil_) {
            x = z->l;
            transplant(z, z->l);
        } else {  // 2 filhos: sucessor assume o lugar de z
            y = minimum(z->r);
            yOrig = y->c;
            x = y->r;
            if (y->p == z) {
                x->p = y;
            } else {
                transplant(y, y->r);
                y->r = z->r;
                y->r->p = y;
            }
            transplant(z, y);
            y->l = z->l;
            y->l->p = y;
            paint(y, z->c);  // y herda a cor de z
        }
        delete z;
        --n_;
        ++stats_.removesOk;
        if (yOrig == BLACK) deleteFix(x);
        nil_->p = nil_;  // a sentinela pode ter sido usada como "x"
        return true;
    }

    bool contains(Key k) const override { return find(k) != nil_; }

    std::vector<Key> inorder() const override {
        std::vector<Key> v;
        v.reserve(n_);
        collect(root_, v);
        return v;
    }

    std::size_t size() const override { return n_; }
    int height() const override { return heightOf(root_); }

    double avgDepth() const override {
        if (root_ == nil_) return 0.0;
        long long sum = 0;
        depthSum(root_, 1, sum);
        return static_cast<double>(sum) / static_cast<double>(n_);
    }

    void print(std::ostream& os, bool ansi = false, int maxDepth = 0) const override {
        if (root_ == nil_) {
            os << "(árvore vazia)\n";
            return;
        }
        os << label(root_, ansi) << "\n";
        printChildren(os, root_, "", 1, maxDepth, ansi);
    }

    std::string toDot() const override {
        std::ostringstream o;
        o << "digraph RB {\n  graph [ordering=out];\n  node [shape=circle, fontname=\"Helvetica\"];\n";
        int id = 0;
        if (root_ != nil_) dotRec(root_, o, id);
        o << "}\n";
        return o.str();
    }

    std::string validate() const override {
        if (nil_->c != BLACK) return "propriedade 3 violada: sentinela nil não é negra";
        if (root_ != nil_) {
            if (root_->c != BLACK) return "propriedade 2 violada: a raiz é rubra";
            if (root_->p != nil_) return "o pai da raiz deveria ser nil";
        }
        std::string err;
        std::size_t cnt = 0;
        check(root_, nullptr, nullptr, err, cnt);
        if (err.empty() && cnt != n_) err = "contador de nós inconsistente com a estrutura";
        return err;
    }

    void clear() override {
        destroy(root_);
        root_ = nil_;
        nil_->p = nil_;
        n_ = 0;
    }

    int childMask(Key k) const override {
        const Node* x = find(k);
        if (x == nil_) return -1;
        return (x->l != nil_ ? 1 : 0) | (x->r != nil_ ? 2 : 0);
    }

    bool rootKey(Key& out) const override {
        if (root_ == nil_) return false;
        out = root_->key;
        return true;
    }

    char colorOf(Key k) const override {
        const Node* x = find(k);
        if (x == nil_) return '-';
        return x->c == RED ? 'R' : 'N';
    }
};
