# Build: make            -> ./montador and ./ligador
# Tests: make test       -> runs scripts/run_tests.sh
# Report: make report    -> report/main.pdf (English) and report/main_pt_br.pdf (pt-BR)
#                           (requires pdflatex + latexmk + IEEEtran)

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
SRC       = src

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif

MONTADOR_OBJS = $(SRC)/montador_main.o $(SRC)/assembler.o $(SRC)/lexer.o $(SRC)/common.o $(SRC)/objfile.o
LIGADOR_OBJS  = $(SRC)/ligador_main.o $(SRC)/linker.o $(SRC)/objfile.o $(SRC)/assembler.o $(SRC)/lexer.o $(SRC)/common.o

all: montador$(EXE) ligador$(EXE)

montador$(EXE): $(MONTADOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

ligador$(EXE): $(LIGADOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SRC)/%.o: $(SRC)/%.cpp $(wildcard $(SRC)/*.hpp)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: all
	bash scripts/run_tests.sh

report:
	cd report && latexmk -pdf -interaction=nonstopmode main.tex main_pt_br.tex

clean:
	rm -f $(SRC)/*.o montador$(EXE) ligador$(EXE)

.PHONY: all test report clean
