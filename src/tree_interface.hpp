// Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
// =============================================================================
// tree_interface.hpp
// Interface comum às duas árvores balanceadas (AVL e Rubro-Negra).
//
// Ter uma interface única permite:
//   * escolher a estrutura em tempo de execução (menu do programa);
//   * aplicar exatamente os mesmos testes e as mesmas sequências de operações
//     nas duas árvores (comparação justa).
//
// Convenção de altura adotada em TODO o projeto:
//   altura = número de NÓS no maior caminho da raiz até uma folha.
//   árvore vazia = 0, árvore com um único nó = 1.
// =============================================================================
#pragma once

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

// Identificador de encomenda (ex.: código numérico do pedido).
using Key = long long;

// Contadores acumulados desde a criação (ou desde o último resetStats()).
struct Stats {
    long long insertsOk = 0;       // inserções que alteraram a árvore
    long long insertsDup = 0;      // tentativas de inserir valor repetido (ignoradas)
    long long removesOk = 0;       // remoções efetivas
    long long removesMissing = 0;  // remoções de elemento inexistente (ignoradas)

    // Cada rotação simples conta 1. Uma rotação dupla (LR/RL) conta 2.
    long long rotations = 0;

    // Rubro-Negra: número de vezes que a cor de um nó foi efetivamente trocada.
    long long recolorings = 0;

    // AVL: quantas vezes cada caso de desbalanceamento foi tratado.
    long long avlLL = 0, avlRR = 0, avlLR = 0, avlRL = 0;
};

class ITree {
public:
    using Trace = std::function<void(const std::string&)>;

    virtual ~ITree() = default;

    virtual std::string name() const = 0;

    // Retorna true se inseriu; false se o valor já existia (duplicado ignorado).
    virtual bool insert(Key k) = 0;
    // Retorna true se removeu; false se o valor não existia (nada é alterado).
    virtual bool remove(Key k) = 0;
    virtual bool contains(Key k) const = 0;

    // Elementos em ordem crescente.
    virtual std::vector<Key> inorder() const = 0;

    virtual std::size_t size() const = 0;
    virtual int height() const = 0;
    // Profundidade média dos nós (raiz = 1): custo médio de uma busca bem-sucedida.
    virtual double avgDepth() const = 0;

    // Impressão textual. ansi=true pinta de vermelho os nós rubros (Rubro-Negra).
    // maxDepth > 0 limita os níveis exibidos (útil para árvores grandes).
    virtual void print(std::ostream& os, bool ansi = false, int maxDepth = 0) const = 0;

    // Exporta a árvore no formato DOT (Graphviz).
    virtual std::string toDot() const = 0;

    // Verifica TODAS as invariantes da estrutura. Retorna "" se estiver tudo
    // correto, ou uma mensagem descrevendo a primeira violação encontrada.
    virtual std::string validate() const = 0;

    virtual void clear() = 0;

    // --- Ganchos usados pelos testes (inspeção estrutural) -------------------
    // Bit 0 = tem filho esquerdo, bit 1 = tem filho direito; -1 se k não existe.
    virtual int childMask(Key k) const = 0;
    // Preenche 'out' com a chave da raiz; false se a árvore está vazia.
    virtual bool rootKey(Key& out) const = 0;
    // 'R' (rubro) ou 'N' (negro); '-' se a estrutura não tem cores / k não existe.
    virtual char colorOf(Key) const { return '-'; }

    const Stats& stats() const { return stats_; }
    void resetStats() { stats_ = Stats{}; }

    // Se definido, a árvore descreve cada rotação/recoloração que realiza.
    void setTrace(Trace t) { trace_ = std::move(t); }

protected:
    bool tracing() const { return static_cast<bool>(trace_); }
    void note(const std::string& s) const {
        if (trace_) trace_(s);
    }

    Stats stats_;
    Trace trace_;
};
