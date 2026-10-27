// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// compare.cpp - Experimento comparativo AVL x Rubro-Negra (x BST sem balanceamento)
//
// Para cada conjunto de dados e cada tamanho N:
//   FASE 1 (inserção): insere os N identificadores, na ordem de chegada do conjunto.
//   FASE 2 (remoção) : remove N/2 identificadores, escolhidos numa ordem
//                      pseudoaleatória FIXA (semente), a mesma para as duas árvores.
// As duas estruturas recebem EXATAMENTE a mesma sequência de operações.
// Ao fim de cada fase registramos: altura, altura mínima teórica, profundidade
// média, rotações e recolorações da fase, e (AVL) os casos LL/RR/LR/RL.
// A BST comum entra só como linha de base da fase 1.
//
// Uso: comparar [arquivo.csv]   (também imprime tabelas legíveis na saída padrão)
// =============================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "avl_tree.hpp"
#include "bst_baseline.hpp"
#include "rb_tree.hpp"

namespace {

constexpr unsigned long long kSeed = 42;

struct Dataset {
    std::string name;
    std::vector<Key> (*make)(int n);
};

std::vector<Key> mkAsc(int n) {
    std::vector<Key> v(n);
    for (int i = 0; i < n; ++i) v[i] = i + 1;
    return v;
}
std::vector<Key> mkDesc(int n) {
    auto v = mkAsc(n);
    std::reverse(v.begin(), v.end());
    return v;
}
std::vector<Key> mkRandom(int n) {
    auto v = mkAsc(n);
    std::mt19937_64 rng(kSeed);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}
// Alterna extremos: 1, n, 2, n-1, ... (padrão adversário clássico para BST)
std::vector<Key> mkZigzag(int n) {
    std::vector<Key> v;
    v.reserve(n);
    for (Key lo = 1, hi = n; lo <= hi; ++lo, --hi) {
        v.push_back(lo);
        if (lo != hi) v.push_back(hi);
    }
    return v;
}
// Lotes de 64 identificadores em ordem crescente; a ordem dos lotes é embaralhada.
// Modela pedidos numerados sequencialmente que chegam em pacotes fora de ordem.
std::vector<Key> mkBatches(int n) {
    const int B = 64;
    std::vector<int> blocks;
    for (int s = 0; s < n; s += B) blocks.push_back(s);
    std::mt19937_64 rng(kSeed);
    std::shuffle(blocks.begin(), blocks.end(), rng);
    std::vector<Key> v;
    v.reserve(n);
    for (int s : blocks)
        for (int i = s; i < std::min(n, s + B); ++i) v.push_back(i + 1);
    return v;
}

struct Row {
    std::string dataset;
    int n;
    std::string phase;
    std::string tree;
    std::size_t size;
    int height;
    int minHeight;
    double avgDepth;
    long long rotations;
    long long recolorings;
    long long LL, RR, LR, RL;
    long long ops;
    long long maxRotPerOp;  // máximo de rotações causadas por UMA única operação da fase
};

int minHeightFor(std::size_t n) { return n == 0 ? 0 : static_cast<int>(std::ceil(std::log2(static_cast<double>(n) + 1.0))); }

Stats diff(const Stats& a, const Stats& b) {
    Stats d;
    d.rotations = a.rotations - b.rotations;
    d.recolorings = a.recolorings - b.recolorings;
    d.avlLL = a.avlLL - b.avlLL;
    d.avlRR = a.avlRR - b.avlRR;
    d.avlLR = a.avlLR - b.avlLR;
    d.avlRL = a.avlRL - b.avlRL;
    return d;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string csvPath = argc > 1 ? argv[1] : "";
    const std::vector<Dataset> datasets = {
        {"crescente", mkAsc},
        {"decrescente", mkDesc},
        {"aleatória", mkRandom},
        {"em lotes (64 crescentes, lotes embaralhados)", mkBatches},
        {"alternada (extremos)", mkZigzag},
    };
    const std::vector<int> sizes = {1000, 10000, 100000};

    std::vector<Row> rows;
    struct BaseRow {
        std::string dataset;
        int n;
        int height;  // -1 = não calculado
        double avgDepth;
    };
    std::vector<BaseRow> base;

    for (const auto& ds : datasets) {
        for (int n : sizes) {
            const auto keys = ds.make(n);
            // Ordem de remoção: N/2 chaves, pseudoaleatória e idêntica para as duas árvores.
            auto order = mkAsc(n);
            std::mt19937_64 rng(kSeed + 1);
            std::shuffle(order.begin(), order.end(), rng);
            order.resize(static_cast<std::size_t>(n / 2));

            // --- linha de base: BST comum (só inserção). Em dados quase ordenados é
            //     O(n^2), então só medimos quando é viável (n <= 10000) ou quando o
            //     conjunto não é degenerado.
            {
                const bool degenerate = (ds.name == "crescente" || ds.name == "decrescente" || ds.name == "alternada (extremos)");
                if (!(degenerate && n > 10000)) {
                    BaselineBST b;
                    for (Key k : keys) b.insert(k);
                    base.push_back({ds.name, n, b.height(), b.avgDepth()});
                } else {
                    base.push_back({ds.name, n, -1, 0.0});
                }
            }

            std::unique_ptr<ITree> trees[2] = {std::make_unique<AVLTree>(), std::make_unique<RBTree>()};
            for (auto& t : trees) {
                // FASE 1
                Stats s0 = t->stats();
                long long maxIns = 0;
                for (Key k : keys) {
                    const long long r0 = t->stats().rotations;
                    t->insert(k);
                    maxIns = std::max(maxIns, t->stats().rotations - r0);
                }
                Stats d1 = diff(t->stats(), s0);
                const std::string err1 = t->validate();
                if (!err1.empty()) {
                    std::cerr << "INVARIANTE VIOLADA (" << t->name() << ", " << ds.name << ", n=" << n << "): " << err1 << "\n";
                    return 1;
                }
                rows.push_back({ds.name, n, "inserção", t->name(), t->size(), t->height(), minHeightFor(t->size()), t->avgDepth(),
                                d1.rotations, d1.recolorings, d1.avlLL, d1.avlRR, d1.avlLR, d1.avlRL, static_cast<long long>(keys.size()), maxIns});
                // FASE 2
                Stats s1 = t->stats();
                long long maxRem = 0;
                for (Key k : order) {
                    const long long r0 = t->stats().rotations;
                    t->remove(k);
                    maxRem = std::max(maxRem, t->stats().rotations - r0);
                }
                Stats d2 = diff(t->stats(), s1);
                const std::string err2 = t->validate();
                if (!err2.empty()) {
                    std::cerr << "INVARIANTE VIOLADA após remoções (" << t->name() << ", " << ds.name << ", n=" << n << "): " << err2 << "\n";
                    return 1;
                }
                rows.push_back({ds.name, n, "remoção", t->name(), t->size(), t->height(), minHeightFor(t->size()), t->avgDepth(),
                                d2.rotations, d2.recolorings, d2.avlLL, d2.avlRR, d2.avlLR, d2.avlRL, static_cast<long long>(order.size()), maxRem});
            }
        }
    }

    // ------------------------------------------------------- tempos reais
    // N = 100000; cada medida é a MEDIANA de 5 execuções (cada execução parte de
    // uma árvore vazia). Mede: inserir N chaves, 1.000.000 de buscas bem-sucedidas
    // (chaves sorteadas, mesma sequência nas duas árvores) e remover N/2 chaves.
    // Tempos dependem da máquina; servem como ordem de grandeza, não como valor absoluto.
    struct TimeRow {
        std::string dataset;
        std::string tree;
        double insMs, lookMs, remMs;
    };
    std::vector<TimeRow> times;
    {
        using Clock = std::chrono::steady_clock;
        auto ms = [](Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
        auto median = [](std::vector<double> v) {
            std::sort(v.begin(), v.end());
            return v[v.size() / 2];
        };
        const int n = 100000, reps = 11, lookups = 1000000;
        volatile long long sink = 0;
        for (const auto& ds : datasets) {
            const auto keys = ds.make(n);
            auto order = mkAsc(n);
            std::mt19937_64 rng(kSeed + 1);
            std::shuffle(order.begin(), order.end(), rng);
            order.resize(static_cast<std::size_t>(n / 2));
            std::vector<Key> probes(static_cast<std::size_t>(lookups));
            std::mt19937_64 rng2(kSeed + 2);
            std::uniform_int_distribution<int> pick(1, n);
            for (auto& p : probes) p = pick(rng2);
            for (int which = 0; which < 2; ++which) {
                std::vector<double> ti, tl, tr;
                std::string tname;
                for (int r = 0; r < reps; ++r) {
                    std::unique_ptr<ITree> t = which == 0 ? std::unique_ptr<ITree>(new AVLTree()) : std::unique_ptr<ITree>(new RBTree());
                    tname = t->name();
                    auto a = Clock::now();
                    for (Key k : keys) t->insert(k);
                    auto b = Clock::now();
                    long long hits = 0;
                    for (Key p : probes) hits += t->contains(p) ? 1 : 0;
                    auto c = Clock::now();
                    for (Key k : order) t->remove(k);
                    auto d = Clock::now();
                    sink = sink + hits;
                    ti.push_back(ms(a, b));
                    tl.push_back(ms(b, c));
                    tr.push_back(ms(c, d));
                }
                times.push_back({ds.name, tname, median(ti), median(tl), median(tr)});
            }
        }
    }

    // ------------------------------------------------------------- CSV
    if (!csvPath.empty()) {
        std::ofstream f(csvPath);
        f << "conjunto,N,fase,arvore,nos,altura,altura_minima,prof_media,rotacoes,recoloracoes,LL,RR,LR,RL,operacoes,max_rot_por_op\n";
        for (const auto& r : rows) {
            f << '"' << r.dataset << '"' << ',' << r.n << ',' << r.phase << ',' << r.tree << ',' << r.size << ',' << r.height << ','
              << r.minHeight << ',' << std::fixed << std::setprecision(3) << r.avgDepth << ',' << r.rotations << ',' << r.recolorings
              << ',' << r.LL << ',' << r.RR << ',' << r.LR << ',' << r.RL << ',' << r.ops << ',' << r.maxRotPerOp << '\n';
        }
        f << "\n# linha de base: BST comum sem balanceamento (só inserção; -1 = não medido por custo O(n^2))\n";
        f << "conjunto,N,altura_bst,prof_media_bst\n";
        for (const auto& b : base) f << '"' << b.dataset << '"' << ',' << b.n << ',' << b.height << ',' << std::fixed << std::setprecision(3) << b.avgDepth << '\n';
        f << "\n# tempos (ms), N=100000, mediana de 11 execuções; buscas = 1.000.000 de buscas bem-sucedidas\n";
        f << "conjunto,arvore,insercao_ms,busca_1M_ms,remocao_ms\n";
        for (const auto& t : times)
            f << '"' << t.dataset << '"' << ',' << t.tree << ',' << std::fixed << std::setprecision(2) << t.insMs << ',' << t.lookMs << ',' << t.remMs << '\n';
    }

    // ------------------------------------------------------- tabelas legíveis
    std::cout << "Experimento comparativo AVL x Rubro-Negra (semente fixa = " << kSeed << ")\n";
    std::cout << "Altura em NÓS no maior caminho; mínima = ceil(log2(n+1)); rotação dupla conta 2; Máx/op = máximo de rotações numa única operação.\n";
    std::cout << "Fase 1 = inserir N chaves na ordem do conjunto; Fase 2 = remover N/2 chaves (ordem aleatória fixa).\n";

    std::size_t bi = 0;
    std::string last;
    for (std::size_t i = 0; i < rows.size(); i += 4) {  // 4 linhas por (conjunto,N): AVL ins, AVL rem, RB ins, RB rem
        const Row& ai = rows[i];
        const Row& ar = rows[i + 1];
        const Row& ri = rows[i + 2];
        const Row& rr = rows[i + 3];
        if (ai.dataset != last) {
            std::cout << "\n=== Conjunto: " << ai.dataset << " ===\n";
            last = ai.dataset;
        }
        const BaseRow& b = base[bi++];
        std::cout << "\nN = " << ai.n << "   (altura mínima após inserção = " << ai.minHeight << ")\n";
        std::cout << std::left << std::setw(14) << "Árvore" << std::right << std::setw(10) << "Fase" << std::setw(10) << "Nós"
                  << std::setw(9) << "Altura" << std::setw(11) << "Prof.méd" << std::setw(11) << "Rotações" << std::setw(13) << "Recolor."
                  << std::setw(12) << "Rot/op" << std::setw(12) << "Máx/op" << "\n";
        auto line = [&](const Row& r) {
            std::cout << std::left << std::setw(14) << r.tree << std::right << std::setw(10) << r.phase << std::setw(10) << r.size << std::setw(9)
                      << r.height << std::setw(11) << std::fixed << std::setprecision(2) << r.avgDepth << std::setw(11) << r.rotations
                      << std::setw(13) << (r.tree == "AVL" ? std::string("-") : std::to_string(r.recolorings)) << std::setw(12)
                      << std::setprecision(3) << (static_cast<double>(r.rotations) / static_cast<double>(r.ops)) << std::setw(12) << r.maxRotPerOp << "\n";
        };
        line(ai);
        line(ar);
        line(ri);
        line(rr);
        std::cout << "AVL (inserção): LL=" << ai.LL << " RR=" << ai.RR << " LR=" << ai.LR << " RL=" << ai.RL << "   |   AVL (remoção): LL=" << ar.LL
                  << " RR=" << ar.RR << " LR=" << ar.LR << " RL=" << ar.RL << "\n";
        if (b.height >= 0)
            std::cout << "BST sem balanceamento (inserção): altura " << b.height << ", profundidade média " << std::fixed << std::setprecision(2) << b.avgDepth << "\n";
        else
            std::cout << "BST sem balanceamento (inserção): não medida (custo O(n^2) neste conjunto)\n";
    }

    std::cout << "\n=== Tempos de execução (N = 100000; mediana de 11 execuções; ordem de grandeza, dependem da máquina) ===\n";
    std::cout << std::left << std::setw(48) << "Conjunto" << std::setw(14) << "Árvore" << std::right << std::setw(14) << "Inserir (ms)" << std::setw(18)
              << "1M buscas (ms)" << std::setw(15) << "Remover (ms)" << "\n";
    for (const auto& t : times)
        std::cout << std::left << std::setw(48) << t.dataset << std::setw(14) << t.tree << std::right << std::fixed << std::setprecision(1) << std::setw(14)
                  << t.insMs << std::setw(18) << t.lookMs << std::setw(15) << t.remMs << "\n";
    return 0;
}
