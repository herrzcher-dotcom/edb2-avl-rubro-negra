# Amazon Engineering Challenge - Árvores Balanceadas (AVL e Rubro-Negra)
# Requer: g++ com suporte a C++17 e make.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
INC      := -Isrc
HEADERS  := $(wildcard src/*.hpp)

all: bin/arvores bin/testes bin/comparar

bin/arvores: src/main.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $<

bin/testes: tests/test_trees.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $<

bin/comparar: tests/compare.cpp $(HEADERS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $<

# Roda a suíte de testes (retorna código != 0 se algum teste falhar).
test: bin/testes
	./bin/testes

# Roda o experimento comparativo AVL x Rubro-Negra (grava results/comparacao.csv).
comparar: bin/comparar
	@mkdir -p results
	./bin/comparar results/comparacao.csv

# Regenera TODAS as evidências em results/ (testes, comparação e demonstrações).
results: all
	@mkdir -p results
	./bin/testes > results/testes.txt
	./bin/comparar results/comparacao.csv > results/comparacao.txt
	./bin/arvores --avl --eco --trace --compacto < demo/demo_avl.in > results/demo_avl.txt
	./bin/arvores --rb  --eco --trace --compacto < demo/demo_rb.in  > results/demo_rb.txt

# Binários estáticos (rodam em qualquer Linux x86-64, sem depender de libstdc++ instalada).
estatico:
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) -static -o bin/arvores src/main.cpp
	$(CXX) $(CXXFLAGS) $(INC) -static -o bin/testes tests/test_trees.cpp
	$(CXX) $(CXXFLAGS) $(INC) -static -o bin/comparar tests/compare.cpp

# Teste de mutação: injeta bugs no código e confere se a suíte os detecta (leva alguns minutos).
mutacao:
	python3 tests/mutation_test.py

clean:
	rm -rf bin

.PHONY: all test comparar results estatico mutacao clean
