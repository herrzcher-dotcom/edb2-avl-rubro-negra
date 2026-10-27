// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// test_trees.cpp - Suíte de testes das árvores AVL e Rubro-Negra.
//
// Os MESMOS testes (mesmos dados e mesmas sequências de operações) são
// executados nas duas estruturas. Em quase todos os testes, além de comparar o
// resultado com um oráculo (std::set), chamamos validate() APÓS CADA OPERAÇÃO,
// que confere todas as invariantes:
//   AVL:         ordem BST, alturas armazenadas corretas e |fb| <= 1 em todo nó;
//   Rubro-Negra: ordem BST, raiz negra, nil negro, sem rubro-rubro, mesma altura
//                negra em todos os caminhos e ponteiros de pai consistentes.
//
// Saída: uma linha por teste ([OK] / [FALHOU]) e um resumo. Código de saída
// diferente de zero se algum teste falhar.
// =============================================================================
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "avl_tree.hpp"
#include "rb_tree.hpp"

using Factory = std::function<std::unique_ptr<ITree>()>;

namespace {

// ------------------------------------------------------------------ infra ---
struct Report {
    int passed = 0;
    int failed = 0;
};

struct Ctx {
    std::vector<std::string> failures;
    std::string info;
    void expect(bool cond, const std::string& msg) {
        if (!cond && failures.size() < 5) failures.push_back(msg);
        if (!cond && failures.size() >= 5 && failures.back() != "...") failures.push_back("...");
    }
    bool ok() const { return failures.empty(); }
};

void runTest(Report& rep, const std::string& treeName, const std::string& title, const std::function<void(Ctx&)>& body) {
    Ctx c;
    body(c);
    if (c.ok()) {
        ++rep.passed;
        std::cout << "  [OK]     " << title;
        if (!c.info.empty()) std::cout << "  -> " << c.info;
        std::cout << "\n";
    } else {
        ++rep.failed;
        std::cout << "  [FALHOU] " << title << "\n";
        for (const auto& f : c.failures) std::cout << "           - " << f << "\n";
    }
    (void)treeName;
}

// Compara a árvore com o oráculo e valida as invariantes. "" = tudo certo.
std::string checkAgainst(const ITree& t, const std::set<Key>& oracle) {
    const std::string err = t.validate();
    if (!err.empty()) return "invariante violada: " + err;
    if (t.size() != oracle.size()) return "tamanho " + std::to_string(t.size()) + " != esperado " + std::to_string(oracle.size());
    const auto v = t.inorder();
    if (!std::equal(v.begin(), v.end(), oracle.begin(), oracle.end())) return "percurso em ordem diferente do esperado";
    return "";
}

// Limites teóricos de altura (em nós):
//   AVL:  h(arestas) < 1,4405*log2(n+2) - 0,3277  => H(nós) < 1,4405*log2(n+2) + 0,6723
//   RB:   h(arestas) <= 2*log2(n+1)               => H(nós) <= 2*log2(n+1) + 1
bool withinTheory(const ITree& t) {
    const double n = static_cast<double>(t.size());
    const int H = t.height();
    if (t.size() == 0) return H == 0;
    if (t.name() == "AVL") return H < 1.4405 * std::log2(n + 2.0) + 0.6723 + 1e-9;
    return (H - 1) <= 2.0 * std::log2(n + 1.0) + 1e-9;
}

std::vector<Key> iota(Key from, Key to) {
    std::vector<Key> v;
    for (Key i = from; i <= to; ++i) v.push_back(i);
    return v;
}

// Insere cada chave validando a cada passo. Retorna "" ou a descrição do problema.
std::string insertAllChecked(ITree& t, std::set<Key>& o, const std::vector<Key>& keys, bool checkTheory = true) {
    for (Key k : keys) {
        const bool expected = o.insert(k).second;
        if (t.insert(k) != expected) return "insert(" + std::to_string(k) + ") retornou valor inesperado";
        const std::string e = checkAgainst(t, o);
        if (!e.empty()) return "após inserir " + std::to_string(k) + ": " + e;
        if (checkTheory && !withinTheory(t)) return "altura " + std::to_string(t.height()) + " acima do limite teórico com n=" + std::to_string(t.size());
    }
    return "";
}

std::string removeAllChecked(ITree& t, std::set<Key>& o, const std::vector<Key>& keys) {
    for (Key k : keys) {
        const bool expected = o.erase(k) > 0;
        if (t.remove(k) != expected) return "remove(" + std::to_string(k) + ") retornou valor inesperado";
        const std::string e = checkAgainst(t, o);
        if (!e.empty()) return "após remover " + std::to_string(k) + ": " + e;
        if (!withinTheory(t)) return "altura " + std::to_string(t.height()) + " acima do limite teórico com n=" + std::to_string(t.size());
    }
    return "";
}

std::vector<Key> shuffled(std::vector<Key> v, unsigned long long seed) {
    std::mt19937_64 rng(seed);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}

// ------------------------------------------------------------ a suíte ---
void suite(Report& rep, const std::string& label, const Factory& mk) {
    std::cout << "\n=== " << label << " ===\n";
    const bool isAVL = (mk()->name() == "AVL");

    // ---- 1. Árvore vazia ---------------------------------------------------
    runTest(rep, label, "Operações em árvore vazia", [&](Ctx& c) {
        auto t = mk();
        c.expect(t->size() == 0, "size() deveria ser 0");
        c.expect(t->height() == 0, "height() deveria ser 0");
        c.expect(!t->contains(42), "contains(42) deveria ser false");
        c.expect(!t->remove(42), "remove(42) deveria ser false");
        c.expect(t->inorder().empty(), "inorder() deveria ser vazio");
        c.expect(t->validate().empty(), "validate() deveria passar");
        c.expect(t->avgDepth() == 0.0, "avgDepth() deveria ser 0");
        Key r;
        c.expect(!t->rootKey(r), "rootKey() deveria ser false");
        std::ostringstream os;
        t->print(os);
        c.expect(os.str().find("vazia") != std::string::npos, "print() deveria informar árvore vazia");
        c.expect(t->stats().removesMissing == 1, "remoção em árvore vazia deveria contar como inexistente");
        c.info = "busca/remoção/percurso/impressão seguros";
    });

    // ---- 2. Árvore com um elemento ----------------------------------------
    runTest(rep, label, "Operações em árvore com um único elemento", [&](Ctx& c) {
        auto t = mk();
        c.expect(t->insert(7), "insert(7) deveria retornar true");
        c.expect(t->size() == 1 && t->height() == 1, "size=1 e altura=1 esperados");
        c.expect(t->contains(7) && !t->contains(8), "busca de 7 (sim) e 8 (não)");
        c.expect(t->inorder() == std::vector<Key>{7}, "inorder = {7}");
        c.expect(t->childMask(7) == 0, "único nó deve ser folha");
        c.expect(!t->insert(7), "inserir 7 de novo deveria falhar (duplicado)");
        c.expect(!t->remove(8), "remove(8) deveria falhar");
        c.expect(t->size() == 1, "size continua 1");
        c.expect(t->validate().empty(), "invariantes ok com 1 nó");
        c.expect(t->remove(7), "remove(7) deveria funcionar");
        c.expect(t->size() == 0 && t->height() == 0 && t->validate().empty(), "volta a ficar vazia e válida");
        c.expect(t->insert(7), "reinserir 7 após esvaziar");
        c.expect(t->validate().empty(), "invariantes ok após reinserir");
        c.info = "insere, duplica, remove, esvazia e reutiliza";
    });

    // ---- 3-5. Inserção em ordem crescente / decrescente / não ordenada ------
    const int N = 2000;
    struct Order {
        std::string title;
        std::vector<Key> keys;
    };
    std::vector<Key> zig;  // alternando extremos: 1, N, 2, N-1, ...
    for (Key lo = 1, hi = N; lo <= hi; ++lo, --hi) {
        zig.push_back(lo);
        if (lo != hi) zig.push_back(hi);
    }
    std::vector<Key> desc = iota(1, N);
    std::reverse(desc.begin(), desc.end());
    const std::vector<Order> orders = {
        {"Inserção em ordem CRESCENTE (n=2000)", iota(1, N)},
        {"Inserção em ordem DECRESCENTE (n=2000)", desc},
        {"Inserção NÃO ORDENADA - permutação aleatória (n=2000)", shuffled(iota(1, N), 123)},
        {"Inserção NÃO ORDENADA - alternando extremos (n=2000)", zig},
    };
    for (const auto& ord : orders) {
        runTest(rep, label, ord.title, [&](Ctx& c) {
            auto t = mk();
            std::set<Key> o;
            const std::string e = insertAllChecked(*t, o, ord.keys);
            c.expect(e.empty(), e);
            c.info = "invariantes OK após cada inserção; altura final " + std::to_string(t->height()) + ", rotações " +
                     std::to_string(t->stats().rotations);
        });
    }

    // ---- 6. Inserção de valores repetidos ---------------------------------
    runTest(rep, label, "Inserção de valores repetidos (ignorados)", [&](Ctx& c) {
        auto t = mk();
        std::set<Key> o;
        const auto keys = shuffled(iota(1, 300), 5);
        std::string e = insertAllChecked(*t, o, keys);
        c.expect(e.empty(), e);
        const auto before = t->inorder();
        const long long rotBefore = t->stats().rotations;
        for (Key k : keys) c.expect(!t->insert(k), "insert(" + std::to_string(k) + ") duplicado deveria retornar false");
        c.expect(t->inorder() == before, "conteúdo mudou após inserir duplicados");
        c.expect(t->stats().insertsDup == 300, "contador de duplicados deveria ser 300");
        c.expect(t->stats().rotations == rotBefore, "duplicados não devem causar rotações");
        c.expect(t->validate().empty(), "invariantes ok");
        c.info = "300 duplicados rejeitados, árvore inalterada, 0 rotações extras";
    });

    // ---- 7. Busca de existentes e inexistentes ----------------------------
    runTest(rep, label, "Busca de elementos existentes e inexistentes", [&](Ctx& c) {
        auto t = mk();
        for (Key k = 1; k <= 1999; k += 2) t->insert(k);  // só ímpares
        int found = 0, missing = 0;
        for (Key k = 1; k <= 1999; k += 2) {
            c.expect(t->contains(k), "deveria encontrar " + std::to_string(k));
            ++found;
        }
        for (Key k = 0; k <= 2000; k += 2) {
            c.expect(!t->contains(k), "não deveria encontrar " + std::to_string(k));
            ++missing;
        }
        c.expect(!t->contains(-5) && !t->contains(1000000000LL), "valores fora do intervalo");
        c.info = std::to_string(found) + " buscas com sucesso e " + std::to_string(missing) + " sem sucesso";
    });

    // ---- 8. Remoção de nós com todas as configurações de filhos -----------
    runTest(rep, label, "Remoção de nós: folha, 1 filho (esq.), 1 filho (dir.), 2 filhos e raiz", [&](Ctx& c) {
        long long cnt[4] = {0, 0, 0, 0};
        long long rootRemovals = 0;
        for (unsigned long long seed = 1; seed <= 6; ++seed) {
            const auto order = shuffled(iota(1, 40), seed);
            for (Key target = 1; target <= 40; ++target) {
                auto t = mk();
                std::set<Key> o;
                for (Key k : order) {
                    t->insert(k);
                    o.insert(k);
                }
                const int mask = t->childMask(target);
                Key r = 0;
                t->rootKey(r);
                if (r == target) ++rootRemovals;
                c.expect(mask >= 0 && mask <= 3, "máscara inválida");
                if (mask >= 0 && mask <= 3) ++cnt[mask];
                c.expect(t->remove(target), "remove(" + std::to_string(target) + ") deveria retornar true");
                o.erase(target);
                const std::string e = checkAgainst(*t, o);
                c.expect(e.empty(), "seed " + std::to_string(seed) + ", removendo " + std::to_string(target) + ": " + e);
            }
        }
        c.expect(cnt[0] > 0, "nenhuma folha foi testada");
        c.expect(cnt[1] > 0, "nenhum nó só com filho esquerdo foi testado");
        c.expect(cnt[2] > 0, "nenhum nó só com filho direito foi testado");
        c.expect(cnt[3] > 0, "nenhum nó com 2 filhos foi testado");
        c.expect(rootRemovals > 0, "nenhuma remoção de raiz foi testada");
        c.info = "folhas=" + std::to_string(cnt[0]) + ", só esq.=" + std::to_string(cnt[1]) + ", só dir.=" + std::to_string(cnt[2]) +
                 ", 2 filhos=" + std::to_string(cnt[3]) + ", raiz=" + std::to_string(rootRemovals) + " (" +
                 std::to_string(cnt[0] + cnt[1] + cnt[2] + cnt[3]) + " remoções)";
    });

    // ---- 9. Esvaziar a árvore de várias formas -----------------------------
    runTest(rep, label, "Esvaziar a árvore: ordem crescente, decrescente, aleatória e sempre pela raiz", [&](Ctx& c) {
        const auto base = shuffled(iota(1, 500), 77);
        for (int mode = 0; mode < 4; ++mode) {
            auto t = mk();
            std::set<Key> o;
            std::string e = insertAllChecked(*t, o, base);
            c.expect(e.empty(), e);
            if (mode == 3) {  // remove sempre a raiz atual
                Key r;
                while (t->rootKey(r)) {
                    c.expect(t->remove(r), "remove(raiz) deveria funcionar");
                    o.erase(r);
                    const std::string e2 = checkAgainst(*t, o);
                    if (!e2.empty()) {
                        c.expect(false, "removendo a raiz " + std::to_string(r) + ": " + e2);
                        break;
                    }
                }
            } else {
                std::vector<Key> seq = iota(1, 500);
                if (mode == 1) std::reverse(seq.begin(), seq.end());
                if (mode == 2) seq = shuffled(seq, 999);
                e = removeAllChecked(*t, o, seq);
                c.expect(e.empty(), "modo " + std::to_string(mode) + ": " + e);
            }
            c.expect(t->size() == 0 && t->height() == 0, "deveria terminar vazia");
        }
        c.info = "4 estratégias, invariantes OK após cada remoção, termina vazia";
    });

    // ---- 10. Remoção de inexistentes --------------------------------------
    runTest(rep, label, "Remoção de elementos inexistentes (ignorada, árvore intacta)", [&](Ctx& c) {
        auto t = mk();
        std::set<Key> o;
        std::string e = insertAllChecked(*t, o, shuffled(iota(1, 100), 3));
        c.expect(e.empty(), e);
        const auto before = t->inorder();
        const long long rotBefore = t->stats().rotations;
        long long tries = 0;
        for (Key k : std::vector<Key>{0, -1, 101, 500, 1000000000LL, -1000000000LL}) {
            c.expect(!t->remove(k), "remove(" + std::to_string(k) + ") deveria retornar false");
            ++tries;
        }
        // remover duas vezes o mesmo valor: a 2ª vez é inexistente
        c.expect(t->remove(50), "primeira remoção de 50 funciona");
        c.expect(!t->remove(50), "segunda remoção de 50 deveria falhar");
        ++tries;
        o.erase(50);
        c.expect(t->stats().removesMissing == tries, "contador de inexistentes deveria ser " + std::to_string(tries));
        e = checkAgainst(*t, o);
        c.expect(e.empty(), e);
        c.expect(t->stats().rotations >= rotBefore, "contador de rotações não pode diminuir");
        (void)before;
        c.info = std::to_string(tries) + " remoções de inexistentes rejeitadas sem alterar a estrutura";
    });

    // ---- 11. Impressão -----------------------------------------------------
    runTest(rep, label, "Impressão da estrutura (e cores na Rubro-Negra)", [&](Ctx& c) {
        auto t = mk();
        for (Key k = 1; k <= 15; ++k) t->insert(k);
        std::ostringstream os;
        t->print(os);
        const std::string s = os.str();
        for (Key k = 1; k <= 15; ++k) c.expect(s.find(std::to_string(k)) != std::string::npos, "impressão não contém " + std::to_string(k));
        if (!isAVL) {
            int reds = 0, blacks = 0;
            for (std::size_t p = 0; (p = s.find("(R)", p)) != std::string::npos; ++p) ++reds;
            for (std::size_t p = 0; (p = s.find("(N)", p)) != std::string::npos; ++p) ++blacks;
            c.expect(reds + blacks == 15, "todos os 15 nós deveriam exibir a cor");
            int redsReal = 0;
            for (Key k = 1; k <= 15; ++k) redsReal += (t->colorOf(k) == 'R');
            c.expect(redsReal == reds, "cores impressas diferem das cores reais");
            std::ostringstream os2;
            t->print(os2, true);
            c.expect(os2.str().find("\033[31m") != std::string::npos, "modo ANSI deveria pintar nós rubros");
            c.info = "15 nós: " + std::to_string(reds) + " rubros e " + std::to_string(blacks) + " negros identificados na impressão";
        } else {
            c.expect(s.find("fb=") != std::string::npos && s.find("h=") != std::string::npos, "AVL deveria mostrar h e fb");
            c.info = "15 nós impressos com altura e fator de balanceamento";
        }
        std::ostringstream os3;
        t->print(os3, false, 2);  // limite de níveis
        c.expect(os3.str().find("…") != std::string::npos, "impressão com maxDepth deveria truncar com …");
    });

    // ---- 12. Rotações / ajustes específicos --------------------------------
    if (isAVL) {
        struct Case {
            const char* name;
            std::vector<Key> ins;
            long long Stats::*counter;
            long long rotations;
        };
        const std::vector<Case> cases = {
            {"RR (1,2,3)", {1, 2, 3}, &Stats::avlRR, 1},
            {"LL (3,2,1)", {3, 2, 1}, &Stats::avlLL, 1},
            {"LR (3,1,2)", {3, 1, 2}, &Stats::avlLR, 2},
            {"RL (1,3,2)", {1, 3, 2}, &Stats::avlRL, 2},
        };
        runTest(rep, label, "Os 4 casos de rotação da AVL (LL, RR, LR, RL)", [&](Ctx& c) {
            std::string names;
            for (const auto& cs : cases) {
                auto t = mk();
                for (Key k : cs.ins) t->insert(k);
                Key r = 0;
                t->rootKey(r);
                c.expect(r == 2, std::string(cs.name) + ": a raiz deveria ser 2");
                c.expect(t->height() == 2, std::string(cs.name) + ": altura deveria ser 2");
                c.expect(t->stats().*(cs.counter) == 1, std::string(cs.name) + ": contador do caso deveria ser 1");
                c.expect(t->stats().rotations == cs.rotations, std::string(cs.name) + ": número de rotações incorreto");
                c.expect(t->validate().empty(), std::string(cs.name) + ": invariantes");
                names += std::string(names.empty() ? "" : ", ") + cs.name;
            }
            c.info = names + " -> raiz 2, altura 2";
        });
        runTest(rep, label, "AVL: inserção crescente de 2^k-1 chaves gera árvore perfeita", [&](Ctx& c) {
            for (int k = 1; k <= 11; ++k) {
                auto t = mk();
                const Key n = (1LL << k) - 1;
                for (Key i = 1; i <= n; ++i) t->insert(i);
                c.expect(t->height() == k, "n=" + std::to_string(n) + " deveria ter altura " + std::to_string(k) + ", obteve " + std::to_string(t->height()));
            }
            c.info = "n = 1, 3, 7, ..., 2047 => altura = log2(n+1) exatamente";
        });
    } else {
        runTest(rep, label, "RB: caso 3 (tio negro, linha reta): 1,2,3 -> 1 rotação, raiz 2 negra, 1 e 3 rubros", [&](Ctx& c) {
            auto t = mk();
            for (Key k : {1, 2, 3}) t->insert(k);
            Key r = 0;
            t->rootKey(r);
            c.expect(r == 2, "raiz deveria ser 2");
            c.expect(t->colorOf(2) == 'N' && t->colorOf(1) == 'R' && t->colorOf(3) == 'R', "cores esperadas: 2=N, 1=R, 3=R");
            c.expect(t->stats().rotations == 1, "deveria haver exatamente 1 rotação");
            c.expect(t->validate().empty(), "invariantes");
            c.info = "1 rotação; 2(N) com filhos 1(R) e 3(R)";
        });
        runTest(rep, label, "RB: caso 2+3 (tio negro, joelho): 1,3,2 -> 2 rotações, raiz 2", [&](Ctx& c) {
            auto t = mk();
            for (Key k : {1, 3, 2}) t->insert(k);
            Key r = 0;
            t->rootKey(r);
            c.expect(r == 2, "raiz deveria ser 2");
            c.expect(t->stats().rotations == 2, "deveriam ocorrer 2 rotações");
            c.expect(t->colorOf(2) == 'N' && t->colorOf(1) == 'R' && t->colorOf(3) == 'R', "cores esperadas: 2=N, 1=R, 3=R");
            c.expect(t->validate().empty(), "invariantes");
            c.info = "2 rotações; 2(N) com filhos 1(R) e 3(R)";
        });
        runTest(rep, label, "RB: caso 1 (tio rubro): 2,1,3,0 -> só recoloração, sem rotação", [&](Ctx& c) {
            auto t = mk();
            for (Key k : {2, 1, 3}) t->insert(k);
            const long long rec0 = t->stats().recolorings;
            t->insert(0);
            c.expect(t->stats().rotations == 0, "não deveria haver rotação");
            // pai e tio -> negro, avô (raiz) -> rubro e, no fim, raiz -> negro de novo
            c.expect(t->stats().recolorings - rec0 == 4, "deveriam ocorrer 4 mudanças de cor (pai, tio, avô e raiz)");
            c.expect(t->colorOf(1) == 'N' && t->colorOf(3) == 'N' && t->colorOf(0) == 'R' && t->colorOf(2) == 'N',
                     "cores esperadas: 1=N, 3=N, 0=R, 2=N");
            c.expect(t->validate().empty(), "invariantes");
            c.info = "4 mudanças de cor (pai, tio, avô->rubro, raiz->negra) e 0 rotações";
        });
        runTest(rep, label, "RB: inserção de 1..n gera altura <= 2*log2(n+1) e rotações <= n", [&](Ctx& c) {
            auto t = mk();
            for (Key k = 1; k <= 1000; ++k) t->insert(k);
            c.expect(withinTheory(*t), "altura acima do limite teórico");
            c.expect(t->stats().rotations <= 1000, "inserção faz no máximo 2 rotações por chamada; observadas " + std::to_string(t->stats().rotations));
            c.info = "altura " + std::to_string(t->height()) + ", rotações " + std::to_string(t->stats().rotations);
        });
    }

    // ---- 13. Mistura aleatória validando a cada operação -------------------
    runTest(rep, label, "Mistura aleatória de inserções/remoções/buscas (5 sementes x 20.000 ops, validando a cada op)", [&](Ctx& c) {
        long long ops = 0, maxN = 0;
        for (unsigned long long seed = 1; seed <= 5; ++seed) {
            std::mt19937_64 rng(seed * 7919);
            std::uniform_int_distribution<int> key(0, 399), op(0, 99);
            auto t = mk();
            std::set<Key> o;
            for (int i = 0; i < 20000; ++i, ++ops) {
                const Key k = key(rng);
                const int r = op(rng);
                if (r < 50) {
                    c.expect(t->insert(k) == o.insert(k).second, "insert(" + std::to_string(k) + ") divergiu do oráculo");
                } else if (r < 90) {
                    c.expect(t->remove(k) == (o.erase(k) > 0), "remove(" + std::to_string(k) + ") divergiu do oráculo");
                } else {
                    c.expect(t->contains(k) == (o.count(k) > 0), "contains(" + std::to_string(k) + ") divergiu do oráculo");
                }
                const std::string e = checkAgainst(*t, o);
                if (!e.empty() || !withinTheory(*t)) {
                    c.expect(false, "seed " + std::to_string(seed) + ", op " + std::to_string(i) + ": " + (e.empty() ? "altura acima do limite teórico" : e));
                    break;
                }
                maxN = std::max<long long>(maxN, static_cast<long long>(t->size()));
            }
        }
        c.info = std::to_string(ops) + " operações, n máximo " + std::to_string(maxN) + ", nenhuma violação";
    });

    // ---- 14. Estresse grande ----------------------------------------------
    runTest(rep, label, "Estresse: 400.000 operações em chaves de 0 a 199.999 (validação periódica)", [&](Ctx& c) {
        std::mt19937_64 rng(2026);
        std::uniform_int_distribution<int> key(0, 199999), op(0, 99);
        auto t = mk();
        std::set<Key> o;
        for (int i = 0; i < 400000; ++i) {
            const Key k = key(rng);
            if (op(rng) < 60)
                c.expect(t->insert(k) == o.insert(k).second, "insert divergiu");
            else
                c.expect(t->remove(k) == (o.erase(k) > 0), "remove divergiu");
            if (i % 20000 == 0 || i == 399999) {
                const std::string e = checkAgainst(*t, o);
                if (!e.empty() || !withinTheory(*t)) {
                    c.expect(false, "op " + std::to_string(i) + ": " + (e.empty() ? "altura acima do limite" : e));
                    break;
                }
            }
        }
        c.info = "n final " + std::to_string(t->size()) + ", altura " + std::to_string(t->height());
    });
}

// Os dois tipos devem produzir exatamente o mesmo conjunto de elementos.
void crossSuite(Report& rep) {
    std::cout << "\n=== AVL x Rubro-Negra (equivalência funcional) ===\n";
    runTest(rep, "cross", "Mesma sequência de operações => mesmos resultados e mesmo percurso em ordem", [&](Ctx& c) {
        AVLTree a;
        RBTree r;
        std::mt19937_64 rng(31337);
        std::uniform_int_distribution<int> key(0, 999), op(0, 99);
        for (int i = 0; i < 100000; ++i) {
            const Key k = key(rng);
            const int o = op(rng);
            if (o < 50) {
                c.expect(a.insert(k) == r.insert(k), "insert divergiu entre AVL e RB");
            } else if (o < 90) {
                c.expect(a.remove(k) == r.remove(k), "remove divergiu entre AVL e RB");
            } else {
                c.expect(a.contains(k) == r.contains(k), "contains divergiu entre AVL e RB");
            }
            if (!c.ok()) break;
        }
        c.expect(a.inorder() == r.inorder(), "percursos em ordem diferentes");
        c.expect(a.stats().insertsOk == r.stats().insertsOk && a.stats().removesOk == r.stats().removesOk, "contadores de operações efetivas diferentes");
        c.info = "100.000 operações idênticas; AVL altura " + std::to_string(a.height()) + " x RB altura " + std::to_string(r.height());
    });
}

}  // namespace

int main() {
    Report rep;
    std::cout << "Suíte de testes - árvores AVL e Rubro-Negra\n";
    std::cout << "(mesmos dados e mesmas operações nas duas estruturas; invariantes verificadas após cada operação)\n";
    suite(rep, "Árvore AVL", [] { return std::make_unique<AVLTree>(); });
    suite(rep, "Árvore Rubro-Negra", [] { return std::make_unique<RBTree>(); });
    crossSuite(rep);
    std::cout << "\n--------------------------------------------------------------\n";
    std::cout << "Resultado: " << rep.passed << " de " << (rep.passed + rep.failed) << " testes passaram";
    if (rep.failed) std::cout << " (" << rep.failed << " FALHARAM)";
    std::cout << "\n";
    return rep.failed == 0 ? 0 : 1;
}
