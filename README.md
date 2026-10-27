# Amazon Engineering Challenge: Árvores Balanceadas (AVL e Rubro-Negra)

> **IMD0039, Estruturas de Dados Básicas II** · Trabalho da Unidade II

| | |
|---|---|
| **Autor** | **Raul Souza da Cruz Belmiro**, matrícula **20250026450** |
| **Repositório** | **https://github.com/herrzcher-dotcom/edb2-avl-rubro-negra** |
| **Linguagem** | C++17 (sem bibliotecas de árvores: todo o balanceamento foi implementado do zero) |

Protótipo para o cenário da *Black Friday* da Amazon: o sistema de logística precisa **registrar, localizar e remover identificadores de encomendas** com eficiência, mesmo quando a ordem de chegada dos dados é desfavorável. O programa implementa duas árvores de busca balanceadas, **AVL** e **Rubro-Negra**, e o repositório traz os testes e o experimento que as comparam.

---

## 1. Estrutura do repositório

```
├── src/
│   ├── tree_interface.hpp   interface comum (ITree) + contadores (Stats)
│   ├── avl_tree.hpp         árvore AVL
│   ├── rb_tree.hpp          árvore Rubro-Negra (algoritmo do CLRS, com nó sentinela)
│   ├── bst_baseline.hpp     BST comum, usada APENAS como linha de base nos experimentos
│   └── main.cpp             programa interativo (menu)
├── tests/
│   ├── test_trees.cpp       suíte de testes (35 testes, mesmas operações nas duas árvores)
│   ├── compare.cpp          experimento comparativo (alturas, rotações, recolorações, tempos)
│   └── mutation_test.py     teste de mutação: confere se a suíte detecta bugs injetados
├── demo/                    entradas dos roteiros de demonstração
├── results/                 evidências geradas pelos programas (testes, comparação, demos)
├── bin/                     binários já compilados (Linux x86-64, estáticos)
├── Makefile
└── README.md
```

## 2. Como compilar e executar

Requisitos: `g++` com suporte a C++17 (testado com g++ 13) e `make`.

```bash
make            # compila bin/arvores, bin/testes e bin/comparar
./bin/arvores   # programa interativo: pergunta qual árvore usar
```

Sem `make`: `g++ -std=c++17 -O2 -Isrc -o bin/arvores src/main.cpp`.
A pasta `bin/` já traz binários **estáticos** para Linux x86-64 (`./bin/arvores` roda direto). Em Windows, use WSL ou MSYS2/MinGW com `g++`.

| Comando | O que faz |
|---|---|
| `make test` | roda os 35 testes (código de saída ≠ 0 se algum falhar) |
| `make comparar` | roda o experimento comparativo e grava `results/comparacao.csv` |
| `make results` | regenera **todas** as evidências em `results/` |
| `make mutacao` | teste de mutação (leva alguns minutos) |
| `make estatico` | recompila os binários de forma estática |

### Opções do programa

```
./bin/arvores [--avl | --rb] [--cor] [--trace] [--eco] [--compacto]
```

| Opção | Efeito |
|---|---|
| `--avl` / `--rb` | já inicia com a árvore escolhida (senão o programa pergunta no início) |
| `--cor` | pinta de vermelho os nós rubros na impressão (terminais ANSI) |
| `--trace` | mostra **cada rotação/recoloração** no momento em que acontece |
| `--eco` | repete na saída as linhas lidas (útil com entrada redirecionada) |
| `--compacto` | não repete o menu completo a cada comando |

## 3. Funcionalidades

A árvore é escolhida **em tempo de execução** (menu inicial ou opção 9) e as duas oferecem exatamente as mesmas operações:

| Menu | Funcionalidade |
|---|---|
| 1 | **Inserir** um ou vários identificadores (separados por espaço) |
| 2 | **Remover** um ou vários identificadores |
| 3 | **Buscar** um ou vários identificadores |
| 4 | **Percurso em ordem** (elementos em ordem crescente) |
| 5 | **Imprimir a estrutura** da árvore (na Rubro-Negra, cada nó mostra sua cor) |
| 6 | **Estatísticas**: nós, altura (e mínima possível), profundidade média, rotações, recolorações, casos AVL (LL/RR/LR/RL) |
| 7 | **Verificar invariantes** (confere todas as propriedades da estrutura) |
| 8 | **Carga em lote** de N identificadores em ordem crescente, decrescente ou aleatória |
| 9 | Trocar de árvore (AVL ↔ Rubro-Negra) |
| T | Liga/desliga o *trace* de rotações e recolorações |
| D | Exporta a árvore em **DOT** (Graphviz): `dot -Tpng arvore.dot -o arvore.png` |

### Exemplo de uso (AVL)

```text
Opção: 1
Identificadores a inserir (separados por espaço): 10 20 30 40 50 25
  ✔ 10 inserido
  ✔ 20 inserido
   ↳ AVL: nó 10 com fb=-2 -> caso RR (rotação simples à esquerda em 10)
  ✔ 30 inserido
  ✔ 40 inserido
   ↳ AVL: nó 30 com fb=-2 -> caso RR (rotação simples à esquerda em 30)
  ✔ 50 inserido
   ↳ AVL: nó 20 com fb=-2 e filho direito com fb>0 -> caso RL (rotação à direita em 40, depois à esquerda em 20)
  ✔ 25 inserido
Opção: 5
30 (h=3, fb=0)
├── E: 20 (h=2, fb=0)
│   ├── E: 10 (h=1, fb=0)
│   └── D: 25 (h=1, fb=0)
└── D: 40 (h=2, fb=-1)
    └── D: 50 (h=1, fb=0)
```

### Exemplo de uso (Rubro-Negra)

`(R)` = nó **rubro** (vermelho), `(N)` = nó **negro** (preto). `E`/`D` = filho esquerdo/direito.

```text
20 (N)
├── E: 10 (N)
└── D: 40 (R)
    ├── E: 30 (N)
    │   └── E: 25 (R)
    └── D: 50 (N)
```

As transcrições completas (com *trace*) estão em [`results/demo_avl.txt`](results/demo_avl.txt) e [`results/demo_rb.txt`](results/demo_rb.txt); para reproduzi-las: `make results`.

## 4. Decisões de implementação (documentadas)

| Situação | Decisão |
|---|---|
| **Inserir valor repetido** | A árvore **não muda**; `insert` devolve `false` e o programa avisa que o valor já existe e que a inserção foi ignorada. Duplicados são contados em `Stats::insertsDup`. Não há rotações nem recolorações. |
| **Buscar elemento inexistente** | Devolve `false` / "não encontrado". |
| **Remover elemento inexistente** | A árvore **não muda**; devolve `false` e o programa avisa que o valor não foi encontrado e que nada foi removido. Contado em `Stats::removesMissing`. |
| **Remover nó com 2 filhos** | Usa o **sucessor em ordem** (menor chave da subárvore direita), nas duas árvores. |
| **Entrada inválida** (texto, número fora do intervalo) | O token é reportado e ignorado; o restante da linha é processado. |
| **Tipo da chave** | `long long` (identificador numérico da encomenda). |
| **Convenção de altura** | Número de **nós** no maior caminho raiz→folha. Vazia = 0; um nó = 1. |
| **Contagem de rotações** | Cada rotação **simples conta 1**; uma rotação **dupla (LR/RL) conta 2**. Vale para as duas árvores. |
| **Contagem de recolorações** | Na Rubro-Negra, conta apenas as trocas de cor que **realmente mudam** a cor de um nó (inclui o repintar da raiz). |
| **Impressão** | Árvore na vertical (raiz no topo, `E:`/`D:` indicam o lado). Árvores com mais de 63 nós mostram só os 6 primeiros níveis; a opção **D** exporta tudo. |

### Como cada balanceamento funciona

**AVL** (`src/avl_tree.hpp`): cada nó guarda sua altura. Ao voltar da recursão de uma inserção/remoção, recalcula-se a altura e o fator de balanceamento `fb = h(esq) − h(dir)`. Se `|fb| = 2`, aplica-se uma das 4 rotações: **LL** (rotação simples à direita), **RR** (simples à esquerda), **LR** (esquerda no filho + direita no nó) ou **RL** (direita no filho + esquerda no nó). Invariante: `|fb| ≤ 1` em todo nó ⇒ altura ≤ 1,44·log₂(n+2).

**Rubro-Negra** (`src/rb_tree.hpp`): algoritmo do CLRS (cap. 13) com nó sentinela `nil`. Propriedades: (1) todo nó é rubro ou negro; (2) a raiz é negra; (3) as folhas (`nil`) são negras; (4) filho de nó rubro é negro; (5) todos os caminhos de um nó às folhas têm o mesmo número de nós negros. Ajusta por **recoloração** (tio rubro) e por **rotações** (tio negro), com no máximo 2 rotações por inserção e 3 por remoção. Invariante ⇒ altura ≤ 2·log₂(n+1) (em arestas).

## 5. Testes

`make test` executa **35 testes** (`tests/test_trees.cpp`), os **mesmos** nas duas árvores. Depois de **cada** operação, `validate()` confere todas as invariantes (ordem BST, alturas/fatores na AVL; raiz negra, ausência de rubro-rubro, mesma altura negra e ponteiros de pai na Rubro-Negra) e o conteúdo é comparado com um oráculo (`std::set`).

Cenários exigidos no enunciado e como são cobertos:

| Cenário | Teste |
|---|---|
| Inserção crescente, decrescente e não ordenada | 4 testes (crescente, decrescente, permutação aleatória, alternando extremos), n = 2000, validando a cada inserção |
| Busca de existentes e inexistentes | 1.000 buscas com sucesso e 1.001 sem sucesso |
| Remoção com diferentes configurações de filhos | 240 remoções cobrindo folha, só filho esquerdo, só filho direito, dois filhos e raiz (a máscara de filhos é conferida antes de remover) |
| Remoção de inexistentes | valores fora do intervalo e remoção repetida; árvore intacta |
| Árvore vazia e com um elemento | testes dedicados (incluindo esvaziar e reutilizar) |
| Propriedades após inserções e remoções | 5 sementes × 20.000 operações aleatórias validadas a cada operação; estresse de 400.000 operações; esvaziar em 4 ordens (inclui "sempre a raiz") |
| Casos de rotação / recoloração | AVL: LL, RR, LR, RL; Rubro-Negra: casos 1, 2 e 3 da inserção |
| Limites teóricos de altura | verificados após cada operação |
| Equivalência entre as estruturas | mesma sequência de 100.000 operações ⇒ mesmos resultados e mesmo percurso em ordem |

Resultado: [`results/testes.txt`](results/testes.txt), com **35 de 35 testes passando**.

**Os testes realmente detectam bugs?** `make mutacao` injeta 13 bugs realistas (um por vez, em cópias do código, por exemplo: esquecer de atualizar a altura após uma rotação, não repintar a raiz, não subir o problema no caso 2 da remoção) e confere se a suíte falha. Resultado em [`results/mutacao.txt`](results/mutacao.txt): **13 de 13 detectados** (11 por testes que falham e 2 por loop infinito).

## 6. Experimento comparativo

`make comparar` aplica **exatamente os mesmos dados e as mesmas sequências de operações** às duas árvores (e a uma BST comum, como linha de base) em 5 conjuntos de dados (crescente, decrescente, aleatória, *em lotes* e alternada) com N = 1.000, 10.000 e 100.000: insere N chaves e depois remove N/2 em ordem aleatória fixa (semente 42). Saída completa em [`results/comparacao.txt`](results/comparacao.txt) e [`results/comparacao.csv`](results/comparacao.csv).

Destaques (N = 100.000, altura em nós; altura mínima possível = 17):

| Conjunto | Altura AVL | Altura Rubro-Negra | Rotações/inserção AVL | Rotações/inserção RB |
|---|---:|---:|---:|---:|
| crescente | 17 | 31 | 1,000 | 1,000 |
| decrescente | 17 | 31 | 1,000 | 1,000 |
| aleatória | 20 | 20 | 0,703 | 0,587 |
| em lotes | 19 | 24 | 1,306 | 1,882 |
| alternada | 21 | 31 | 1,625 | 1,500 |

- Uma **BST sem balanceamento** com chaves crescentes chega a **altura = N** (10.000 para N = 10.000), contra 14 (AVL) e 24 (Rubro-Negra).
- A AVL mantém a altura próxima do mínimo; a Rubro-Negra admite alturas maiores (até ~2·log₂ n) em dados ordenados, mas o **máximo de rotações por operação é constante** (≤ 2 na inserção, ≤ 3 na remoção); na AVL uma única remoção chegou a causar 8 rotações.
- Tempos (N = 100.000, mediana de 11 execuções, **dependem da máquina**): a Rubro-Negra foi mais rápida em inserção e remoção nesta implementação; a busca ficou empatada (diferenças dentro do ruído). Parte dessa diferença vem do estilo das implementações (AVL recursiva × Rubro-Negra iterativa), não só do algoritmo.

A análise completa e a recomendação técnica estão na apresentação em PDF.

## 7. Referências

- CORMEN, T. H.; LEISERSON, C. E.; RIVEST, R. L.; STEIN, C. *Algoritmos: teoria e prática* (*Introduction to Algorithms*), cap. 13: Árvores vermelho-preto. (base da implementação Rubro-Negra)
- SZWARCFITER, J. L.; MARKENZON, L. *Estruturas de Dados e seus Algoritmos*, cap. 5: Árvores balanceadas (AVL e Rubro-Negra).
- ADELSON-VELSKY, G. M.; LANDIS, E. M. *An algorithm for the organization of information*, 1962 (árvore AVL).
- GUIBAS, L. J.; SEDGEWICK, R. *A dichromatic framework for balanced trees*, 1978 (árvores rubro-negras).
- Material de aula de IMD0039, Estruturas de Dados Básicas II (Instituto Metrópole Digital).
