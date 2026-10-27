#!/usr/bin/env python3
# Autor: Raul Souza da Cruz Belmiro - Matrícula 20250026450 (IMD0039 - Estruturas de Dados Básicas II)
"""
Teste de mutação da suíte de testes.

Ideia: um teste que nunca falha não prova nada. Aqui injetamos, UM de cada vez,
bugs realistas (típicos de implementações de AVL / Rubro-Negra) em CÓPIAS do
código-fonte, recompilamos a suíte e conferimos se ela detecta o bug.

  * "detectado (testes falharam)" : algum teste reportou [FALHOU];
  * "detectado (não terminou)"    : o bug causou loop infinito (timeout) - a
                                    suíte normal termina em ~2 s;
  * "NÃO DETECTADO"               : a suíte precisa ser melhorada.

Uso:  python3 tests/mutation_test.py        (a partir da raiz do repositório)
Requer g++ (C++17). Não altera o código original: tudo roda em um diretório temporário.
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TIMEOUT_S = 30

# (nome, arquivo, trecho original, trecho com bug)
MUTANTS = [
    ("AVL: limiar de desbalanceamento errado (>2 em vez de >1)", "avl_tree.hpp",
     "if (b > 1) {  // pesado à esquerda", "if (b > 2) {  // pesado à esquerda"),
    ("AVL: esquece de atualizar a altura após a rotação", "avl_tree.hpp",
     "        update(y);  // y agora é filho: atualiza primeiro\n", ""),
    ("AVL: ignora o caso LR (dupla rotação)", "avl_tree.hpp",
     "if (bal(x->l) < 0) {", "if (false) {"),
    ("AVL: remoção não rebalanceia", "avl_tree.hpp",
     "        return ok ? rebalance(x) : x;\n    }\n\n    const Node* find",
     "        return x;\n    }\n\n    const Node* find"),
    ("AVL: duplicado contado como inserido", "avl_tree.hpp",
     "            ok = false;\n            return x;\n        }\n        return ok ? rebalance(x) : x;",
     "            ok = true;\n            return x;\n        }\n        return ok ? rebalance(x) : x;"),
    ("RB: não repinta a raiz de negro após a inserção", "rb_tree.hpp",
     "        paint(root_, BLACK);  // propriedade 2\n", ""),
    ("RB: caso 1 da inserção não pinta o avô de rubro", "rb_tree.hpp",
     "                    paint(g, RED);\n                    z = g;", "                    z = g;"),
    ("RB: caso 2 da inserção sem a rotação", "rb_tree.hpp",
     "                        z = z->p;\n                        rotateLeft(z);", "                        z = z->p;"),
    ("RB: remoção não chama deleteFix", "rb_tree.hpp",
     "if (yOrig == BLACK) deleteFix(x);", "(void)yOrig;"),
    ("RB: sucessor não herda a cor do nó removido", "rb_tree.hpp",
     "            paint(y, z->c);  // y herda a cor de z\n", ""),
    ("RB: deleteFix caso 4 sem herdar a cor do pai", "rb_tree.hpp",
     "                    paint(w, x->p->c);  // caso 4\n", ""),
    ("RB: deleteFix caso 2 não sobe o problema (x = x->p)", "rb_tree.hpp",
     "                    paint(w, RED);\n                    x = x->p;\n                } else {\n                    if (w->r->c == BLACK) {",
     "                    paint(w, RED);\n                } else {\n                    if (w->r->c == BLACK) {"),
    ("RB: transplant não ajusta o pai quando v é nil", "rb_tree.hpp",
     "        v->p = u->p;  // vale mesmo se v == nil_ (CLRS usa isso em deleteFix)", "        if (v != nil_) v->p = u->p;"),
]


def main():
    tmp = tempfile.mkdtemp(prefix="mutacao_")
    results = []
    try:
        for idx, (name, fname, old, new) in enumerate(MUTANTS, 1):
            d = os.path.join(tmp, f"m{idx}")
            os.makedirs(os.path.join(d, "src"))
            os.makedirs(os.path.join(d, "tests"))
            for h in os.listdir(os.path.join(ROOT, "src")):
                if h.endswith(".hpp"):
                    shutil.copy(os.path.join(ROOT, "src", h), os.path.join(d, "src", h))
            shutil.copy(os.path.join(ROOT, "tests", "test_trees.cpp"), os.path.join(d, "tests", "test_trees.cpp"))
            path = os.path.join(d, "src", fname)
            with open(path, encoding="utf-8") as f:
                txt = f.read()
            if old not in txt:
                results.append((name, "ERRO: trecho original não encontrado (mutante desatualizado)"))
                continue
            with open(path, "w", encoding="utf-8") as f:
                f.write(txt.replace(old, new, 1))
            exe = os.path.join(d, "t")
            c = subprocess.run(["g++", "-std=c++17", "-O1", "-I" + os.path.join(d, "src"), "-o", exe,
                                os.path.join(d, "tests", "test_trees.cpp")], capture_output=True, text=True)
            if c.returncode != 0:
                results.append((name, "ERRO de compilação do mutante"))
                continue
            try:
                r = subprocess.run([exe], capture_output=True, text=True, timeout=TIMEOUT_S)
                failed = r.stdout.count("[FALHOU]")
                if r.returncode != 0 and failed:
                    results.append((name, f"detectado (testes falharam: {failed})"))
                else:
                    results.append((name, "NÃO DETECTADO"))
            except subprocess.TimeoutExpired:
                results.append((name, f"detectado (não terminou em {TIMEOUT_S}s: loop infinito)"))
            print(f"[{idx:2d}/{len(MUTANTS)}] {name}: {results[-1][1]}", flush=True)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    detected = sum(1 for _, s in results if s.startswith("detectado"))
    print("\nResumo: %d de %d mutantes detectados pela suíte." % (detected, len(MUTANTS)))
    sys.exit(0 if detected == len(MUTANTS) else 1)


if __name__ == "__main__":
    main()
