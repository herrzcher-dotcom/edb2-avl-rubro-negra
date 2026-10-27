// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// main.cpp - Amazon Engineering Challenge: Árvores Balanceadas (AVL e Rubro-Negra)
// Programa interativo (menu). O usuário escolhe, em tempo de execução, qual
// árvore usar e pode inserir, remover, buscar, percorrer em ordem, imprimir a
// estrutura, ver estatísticas (altura, rotações), verificar invariantes etc.
//
// Opções de linha de comando:
//   --avl | --rb   já inicia com a árvore escolhida (senão, o programa pergunta)
//   --cor          pinta de vermelho (ANSI) os nós rubros na impressão
//   --trace        mostra cada rotação/recoloração no momento em que acontece
//   --eco          repete na saída cada linha lida (útil ao usar entrada redirecionada)
//   --compacto     não repete o menu completo a cada comando (demonstrações)
//   --ajuda        mostra esta ajuda
// =============================================================================
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "avl_tree.hpp"
#include "rb_tree.hpp"

namespace {

struct Options {
    bool color = false;
    bool eco = false;
    bool trace = false;
    bool compact = false;  // não repete o menu a cada comando (útil em demonstrações)
    char kind = 0;  // 'a' = AVL, 'r' = Rubro-Negra, 0 = perguntar
};

bool parseKey(const std::string& tok, Key& out) {
    if (tok.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long long v = std::strtoll(tok.c_str(), &end, 10);
    if (errno == ERANGE || *end != '\0') return false;
    out = v;
    return true;
}

std::vector<std::string> split(const std::string& s) {
    std::istringstream is(s);
    std::vector<std::string> t;
    std::string w;
    while (is >> w) t.push_back(w);
    return t;
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

int minHeight(std::size_t n) {  // menor altura possível para n nós (em nós)
    return n == 0 ? 0 : static_cast<int>(std::ceil(std::log2(static_cast<double>(n) + 1.0)));
}

class App {
public:
    explicit App(Options o) : opt_(o) {}

    int run() {
        if (opt_.kind == 0) {
            if (!chooseTree()) return 0;
        } else {
            tree_ = makeTree(opt_.kind);
            installTrace();
        }
        std::cout << "\n=== Amazon Black Friday - Registro de encomendas (árvores balanceadas) ===\n";
        for (;;) {
            printMenu();
            std::string line;
            if (!ask("Opção: ", line)) break;
            line = trim(line);
            if (line.empty()) continue;
            const char op = static_cast<char>(std::toupper(static_cast<unsigned char>(line[0])));
            switch (op) {
                case '1': doInsert(); break;
                case '2': doRemove(); break;
                case '3': doSearch(); break;
                case '4': doInorder(); break;
                case '5': doPrint(); break;
                case '6': doStats(); break;
                case '7': doValidate(); break;
                case '8': doBatch(); break;
                case '9': if (!chooseTree()) return 0; break;
                case 'T':
                    opt_.trace = !opt_.trace;
                    installTrace();
                    std::cout << "Trace de rotações/recolorações " << (opt_.trace ? "LIGADO" : "desligado") << ".\n";
                    break;
                case 'D': doDot(); break;
                case '0': std::cout << "Até logo!\n"; return 0;
                default: std::cout << "Opção inválida.\n";
            }
        }
        return 0;
    }

private:
    Options opt_;
    std::unique_ptr<ITree> tree_;

    std::unique_ptr<ITree> makeTree(char kind) {
        std::unique_ptr<ITree> t;
        if (kind == 'a')
            t = std::make_unique<AVLTree>();
        else
            t = std::make_unique<RBTree>();
        return t;
    }

    void installTrace() {
        if (!tree_) return;
        if (opt_.trace)
            tree_->setTrace([](const std::string& s) { std::cout << "   ↳ " << s << "\n"; });
        else
            tree_->setTrace(nullptr);
    }

    // Lê uma linha mostrando o prompt. Retorna false em fim de entrada (EOF).
    bool ask(const std::string& prompt, std::string& line) {
        std::cout << prompt << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return false;
        }
        if (opt_.eco) std::cout << line << "\n";
        return true;
    }

    bool chooseTree() {
        for (;;) {
            std::string line;
            if (!ask("Escolha a árvore:  1) AVL   2) Rubro-Negra  -> ", line)) return false;
            line = trim(line);
            if (line == "1" || line == "2") {
                if (tree_ && tree_->size() > 0)
                    std::cout << "(a árvore anterior, com " << tree_->size() << " nós, foi descartada)\n";
                opt_.kind = (line == "1") ? 'a' : 'r';
                tree_ = makeTree(opt_.kind);
                installTrace();
                std::cout << "Árvore " << tree_->name() << " criada (vazia).\n";
                return true;
            }
            std::cout << "Digite 1 ou 2.\n";
        }
    }

    void printMenu() {
        if (opt_.compact) {
            std::cout << "\n[" << tree_->name() << " | nós: " << tree_->size() << " | altura: " << tree_->height() << "]\n";
            return;
        }
        std::cout << "\n--------------------------------------------------------------\n"
                  << " Árvore ativa: " << tree_->name() << "  |  nós: " << tree_->size()
                  << "  |  altura: " << tree_->height() << "\n"
                  << "--------------------------------------------------------------\n"
                  << " 1) Inserir          2) Remover         3) Buscar\n"
                  << " 4) Percurso em ordem                   5) Imprimir árvore\n"
                  << " 6) Estatísticas     7) Verificar invariantes\n"
                  << " 8) Carga em lote (crescente/decrescente/aleatória)\n"
                  << " 9) Trocar de árvore (AVL <-> Rubro-Negra)\n"
                  << " T) Trace de rotações/recolorações [" << (opt_.trace ? "ligado" : "desligado") << "]\n"
                  << " D) Exportar DOT (Graphviz)         0) Sair\n";
    }

    // Lê uma lista de inteiros; tokens inválidos são reportados e ignorados.
    bool askKeys(const std::string& prompt, std::vector<Key>& keys) {
        std::string line;
        if (!ask(prompt, line)) return false;
        keys.clear();
        for (const auto& tok : split(line)) {
            Key k;
            if (parseKey(tok, k))
                keys.push_back(k);
            else
                std::cout << "  ! \"" << tok << "\" não é um identificador válido (inteiro) - ignorado\n";
        }
        if (keys.empty()) std::cout << "Nenhum valor informado.\n";
        return true;
    }

    void doInsert() {
        std::vector<Key> keys;
        if (!askKeys("Identificadores a inserir (separados por espaço): ", keys)) return;
        for (Key k : keys) {
            if (tree_->insert(k))
                std::cout << "  ✔ " << k << " inserido\n";
            else
                std::cout << "  ✘ " << k << " já existe - inserção ignorada (duplicado)\n";
        }
    }

    void doRemove() {
        std::vector<Key> keys;
        if (!askKeys("Identificadores a remover (separados por espaço): ", keys)) return;
        for (Key k : keys) {
            if (tree_->remove(k))
                std::cout << "  ✔ " << k << " removido\n";
            else
                std::cout << "  ✘ " << k << " não encontrado - nada foi removido\n";
        }
    }

    void doSearch() {
        std::vector<Key> keys;
        if (!askKeys("Identificadores a buscar (separados por espaço): ", keys)) return;
        for (Key k : keys)
            std::cout << "  " << (tree_->contains(k) ? "✔ " : "✘ ") << k
                      << (tree_->contains(k) ? " encontrado\n" : " não encontrado\n");
    }

    void doInorder() {
        const auto v = tree_->inorder();
        if (v.empty()) {
            std::cout << "(árvore vazia)\n";
            return;
        }
        std::cout << "Em ordem (" << v.size() << " elementos): ";
        if (v.size() <= 200) {
            for (std::size_t i = 0; i < v.size(); ++i) std::cout << (i ? ", " : "") << v[i];
        } else {
            for (std::size_t i = 0; i < 50; ++i) std::cout << v[i] << ", ";
            std::cout << "… (" << v.size() - 100 << " omitidos) …";
            for (std::size_t i = v.size() - 50; i < v.size(); ++i) std::cout << ", " << v[i];
        }
        std::cout << "\n";
    }

    void doPrint() {
        const int maxDepth = tree_->size() > 63 ? 6 : 0;
        if (maxDepth)
            std::cout << "(árvore grande: exibindo apenas os " << maxDepth << " primeiros níveis; use D para exportar tudo)\n";
        if (tree_->name() != "AVL")
            std::cout << "Legenda: (R) = rubro/vermelho, (N) = negro/preto. E = filho esquerdo, D = filho direito.\n";
        else
            std::cout << "Legenda: h = altura do nó, fb = fator de balanceamento. E = filho esquerdo, D = filho direito.\n";
        tree_->print(std::cout, opt_.color, maxDepth);
    }

    void doStats() {
        const Stats& s = tree_->stats();
        std::cout << "Árvore: " << tree_->name() << "\n"
                  << "Nós: " << tree_->size() << "  |  altura: " << tree_->height()
                  << "  (mínima possível: " << minHeight(tree_->size()) << ")"
                  << "  |  profundidade média: " << std::fixed << std::setprecision(2) << tree_->avgDepth() << "\n"
                  << "Inserções: " << s.insertsOk << " efetivas, " << s.insertsDup << " duplicadas ignoradas\n"
                  << "Remoções:  " << s.removesOk << " efetivas, " << s.removesMissing << " de inexistentes ignoradas\n"
                  << "Rotações (simples; dupla conta 2): " << s.rotations << "\n";
        if (tree_->name() == "AVL")
            std::cout << "Casos AVL tratados: LL=" << s.avlLL << "  RR=" << s.avlRR << "  LR=" << s.avlLR
                      << "  RL=" << s.avlRL << "\n";
        else
            std::cout << "Recolorações: " << s.recolorings << "\n";
    }

    void doValidate() {
        const std::string err = tree_->validate();
        if (err.empty())
            std::cout << "✔ Todas as invariantes da " << tree_->name() << " estão satisfeitas.\n";
        else
            std::cout << "✘ VIOLAÇÃO: " << err << "\n";
    }

    void doBatch() {
        std::string s;
        if (!ask("Quantidade N de identificadores (1 a 1000000): ", s)) return;
        Key n;
        if (!parseKey(trim(s), n) || n < 1 || n > 1000000) {
            std::cout << "Valor inválido.\n";
            return;
        }
        if (!ask("Ordem de chegada: [c]rescente, [d]ecrescente ou [a]leatória: ", s)) return;
        s = trim(s);
        const char o = s.empty() ? ' ' : static_cast<char>(std::tolower(static_cast<unsigned char>(s[0])));
        if (o != 'c' && o != 'd' && o != 'a') {
            std::cout << "Ordem inválida.\n";
            return;
        }
        std::vector<Key> keys(static_cast<std::size_t>(n));
        for (Key i = 0; i < n; ++i) keys[static_cast<std::size_t>(i)] = i + 1;
        if (o == 'd') std::reverse(keys.begin(), keys.end());
        if (o == 'a') {
            std::mt19937_64 rng(42);  // semente fixa: resultado reprodutível
            std::shuffle(keys.begin(), keys.end(), rng);
        }
        const Stats before = tree_->stats();
        for (Key k : keys) tree_->insert(k);
        const Stats& after = tree_->stats();
        std::cout << "Inseridos " << (after.insertsOk - before.insertsOk) << " identificadores ("
                  << (after.insertsDup - before.insertsDup) << " duplicados ignorados). Altura final: "
                  << tree_->height() << " (mínima possível: " << minHeight(tree_->size())
                  << "). Rotações nesta carga: " << (after.rotations - before.rotations) << ".\n";
    }

    void doDot() {
        std::string name;
        if (!ask("Arquivo de saída (Enter = arvore.dot, '-' = tela): ", name)) return;
        name = trim(name);
        if (name.empty()) name = "arvore.dot";
        if (name == "-") {
            std::cout << tree_->toDot();
            return;
        }
        std::ofstream f(name);
        if (!f) {
            std::cout << "Não foi possível gravar em \"" << name << "\".\n";
            return;
        }
        f << tree_->toDot();
        std::cout << "Gravado em " << name << " (renderize com: dot -Tpng " << name << " -o arvore.png)\n";
    }
};

void usage() {
    std::cout << "Uso: arvores [--avl | --rb] [--cor] [--trace] [--eco] [--compacto]\n"
                 "  --avl / --rb  escolhe a árvore ao iniciar (senão o programa pergunta)\n"
                 "  --cor         pinta de vermelho os nós rubros (terminais ANSI)\n"
                 "  --trace       mostra cada rotação/recoloração quando ocorre\n"
                 "  --eco         repete na saída as linhas lidas (entrada redirecionada)\n"
                 "  --compacto    não repete o menu completo a cada comando\n";
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--avl")
            opt.kind = 'a';
        else if (a == "--rb")
            opt.kind = 'r';
        else if (a == "--cor")
            opt.color = true;
        else if (a == "--trace")
            opt.trace = true;
        else if (a == "--eco")
            opt.eco = true;
        else if (a == "--compacto")
            opt.compact = true;
        else if (a == "--ajuda" || a == "-h" || a == "--help") {
            usage();
            return 0;
        } else {
            std::cerr << "Opção desconhecida: " << a << "\n";
            usage();
            return 1;
        }
    }
    App app(opt);
    return app.run();
}
