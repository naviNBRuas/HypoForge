#include "assembler.hpp"

#include <algorithm>
#include <climits>
#include <map>

#include "lexer.hpp"

namespace sb {

namespace {

enum class Section { None, Text, Data };
enum class SymKind { Instruction, Const, Space, Equ, Extern };
enum class Role { Read, Write, Jump };

struct Symbol {
    SymKind kind = SymKind::Instruction;
    Section section = Section::None;
    int address = 0;      // relative to its section until addresses are finalised
    int line = 0;
    long long value = 0;  // CONST / EQU value
    int size = 1;         // words reserved
};

struct Arg {
    std::string name;
    long long offset = 0;  // "LABEL + offset"
    bool valid = true;
};

struct Statement {
    int line = 0;
    const InstructionInfo* info = nullptr;  // nullptr => data directive
    bool isSpace = false;                   // data: SPACE (true) or CONST (false)
    long long number = 0;                   // CONST value or SPACE word count
    std::vector<Arg> args;                  // instruction operands
    bool skipChecks = false;                // the line already had lexical errors
};

Role roleFor(const InstructionInfo& info, std::size_t operandIndex) {
    switch (info.opcode) {
        case 5: case 6: case 7: case 8: return Role::Jump;   // JMP, JMPN, JMPP, JMPZ
        case 11: case 12: return Role::Write;                // STORE, INPUT
        case 9: return operandIndex == 0 ? Role::Read : Role::Write;  // COPY src dst
        default: return Role::Read;
    }
}

class Assembler {
public:
    explicit Assembler(const std::vector<std::string>& lines) : lines_(lines) {}
    AssemblyResult run();

private:
    struct PendingLabel {
        std::string name;
        int line = 0;
        bool valid = false;
    };
    struct PublicDecl {
        std::string name;
        int line = 0;
    };

    // ---- error reporting -------------------------------------------------
    void report(int line, ErrorType type, const std::string& msg) {
        for (const auto& d : diags_)
            if (d.line == line && d.type == type && d.message == msg) return;  // no duplicates
        diags_.push_back({line, type, msg});
    }
    void lexErr(int line, const std::string& m) { report(line, ErrorType::Lexical, m); }
    // A line that already has a lexical error is not analysed further (avoids cascades).
    void synErr(int line, const std::string& m) {
        if (!lineHasLexError_) report(line, ErrorType::Syntactic, m);
    }
    void semErr(int line, const std::string& m) { report(line, ErrorType::Semantic, m); }

    // ---- pass 1: parsing and symbol definition --------------------------
    void parseLine(int lineNo, const std::string& text);
    void handleInstruction(int line, const std::string& label, bool hasLabel,
                           const InstructionInfo* info, const std::vector<Token>& args);
    void handleDirective(int line, const std::string& label, bool hasLabel,
                         const std::string& op, const std::vector<Token>& args);
    void defineSymbol(const std::string& name, const Symbol& sym, int line);
    bool parseOperands(int line, const std::vector<Token>& args, std::vector<Arg>& out);
    std::vector<long long> parseValues(int line, const std::vector<Token>& args, bool& ok);
    void requireNoLabel(int line, bool hasLabel, const std::string& op) {
        if (hasLabel) synErr(line, "a diretiva " + op + " nao aceita rotulo");
    }

    // ---- pass 2: semantic checks and code generation --------------------
    int finalAddress(const Symbol& s) const {
        return s.section == Section::Data ? s.address + textLoc_ : s.address;
    }
    void checkStructure();
    void generate();
    void emitOperand(const Statement& st, std::size_t index, Role role, bool isDiv);
    void push(long long word, int reloc) {
        auto& w = inText_ ? textWords_ : dataWords_;
        auto& r = inText_ ? textReloc_ : dataReloc_;
        w.push_back(word);
        r.push_back(reloc);
    }

    const std::vector<std::string>& lines_;
    std::vector<Diagnostic> diags_;
    std::map<std::string, Symbol> symbols_;
    std::vector<Statement> text_, data_;
    std::vector<PublicDecl> publics_;

    Section current_ = Section::None;
    bool sawText_ = false, sawData_ = false, sawAnySection_ = false;
    int textLoc_ = 0, dataLoc_ = 0;
    PendingLabel pending_;
    bool skipNext_ = false;
    bool lineHasLexError_ = false;

    bool hasBegin_ = false, hasEnd_ = false;
    int beginLine_ = 0, endLine_ = 0;
    std::string moduleName_;

    bool inText_ = true;  // which section generate() is currently emitting
    std::vector<long long> textWords_, dataWords_;
    std::vector<int> textReloc_, dataReloc_;
    std::vector<Use> uses_;
};

// ===========================================================================
// Pass 1
// ===========================================================================

void Assembler::defineSymbol(const std::string& name, const Symbol& sym, int line) {
    if (findInstruction(name) || isDirective(name)) {
        synErr(line, "'" + name + "' e palavra reservada e nao pode ser usada como rotulo");
        return;
    }
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        semErr(line, "rotulo '" + name + "' ja foi definido na linha " + std::to_string(it->second.line));
        return;
    }
    symbols_[name] = sym;
}

// Parses "LABEL [+ N]" operands separated by blanks. Commas are rejected (COPY A B has no comma).
bool Assembler::parseOperands(int line, const std::vector<Token>& a, std::vector<Arg>& out) {
    bool ok = true, commaReported = false;
    std::size_t k = 0;
    while (k < a.size()) {
        const Token& t = a[k];
        if (t.kind == Token::Kind::Comma) {
            if (!commaReported) {
                synErr(line, "virgula nao permitida: os operandos sao separados apenas por espaco");
                commaReported = true;
            }
            ok = false;
            ++k;
        } else if (t.kind == Token::Kind::Ident) {
            Arg arg;
            arg.name = t.text;
            ++k;
            if (k < a.size() && a[k].kind == Token::Kind::Plus) {
                ++k;
                if (k < a.size() && a[k].kind == Token::Kind::Number) {
                    arg.offset = a[k].value;
                    ++k;
                } else {
                    synErr(line, "'+' deve ser seguido de um numero (ex.: X + 2)");
                    ok = false;
                }
            }
            out.push_back(arg);
        } else if (t.kind == Token::Kind::Number) {
            synErr(line, "operando numerico '" + t.text + "' invalido: esperado um rotulo");
            Arg bad;
            bad.valid = false;
            out.push_back(bad);
            ok = false;
            ++k;
        } else {
            synErr(line, "simbolo inesperado '" + t.text + "' na lista de operandos");
            ok = false;
            ++k;
        }
    }
    return ok;
}

// Parses numeric arguments: [+|-] NUMBER, or the name of a previously defined EQU.
std::vector<long long> Assembler::parseValues(int line, const std::vector<Token>& a, bool& ok) {
    std::vector<long long> values;
    std::size_t k = 0;
    ok = true;
    while (k < a.size()) {
        const Token& t = a[k];
        if (t.kind == Token::Kind::Plus || t.kind == Token::Kind::Minus) {
            const bool neg = t.kind == Token::Kind::Minus;
            ++k;
            if (k < a.size() && a[k].kind == Token::Kind::Number) {
                values.push_back(neg ? -a[k].value : a[k].value);
                ++k;
            } else {
                synErr(line, "sinal '" + t.text + "' deve ser seguido de um numero");
                ok = false;
            }
        } else if (t.kind == Token::Kind::Number) {
            values.push_back(t.value);
            ++k;
        } else if (t.kind == Token::Kind::Ident) {
            auto it = symbols_.find(t.text);
            if (it != symbols_.end() && it->second.kind == SymKind::Equ) {
                values.push_back(it->second.value);
            } else {
                semErr(line, "'" + t.text + "' nao e uma constante EQU definida");
                values.push_back(0);
                ok = false;
            }
            ++k;
        } else {
            synErr(line, "simbolo inesperado '" + t.text + "' no argumento");
            ok = false;
            ++k;
        }
    }
    return values;
}

void Assembler::handleInstruction(int line, const std::string& label, bool hasLabel,
                                  const InstructionInfo* info, const std::vector<Token>& args) {
    if (current_ == Section::None) {
        semErr(line, "instrucao " + std::string(info->mnemonic) + " fora de qualquer secao (use SECTION TEXT)");
    } else if (current_ == Section::Data) {
        semErr(line, "instrucao " + std::string(info->mnemonic) + " na secao DATA (so permitida em TEXT)");
    }

    if (hasLabel) {
        Symbol s;
        s.kind = SymKind::Instruction;
        s.section = Section::Text;
        s.address = textLoc_;
        s.line = line;
        defineSymbol(label, s, line);
    }

    Statement st;
    st.line = line;
    st.info = info;
    st.skipChecks = lineHasLexError_;
    const std::size_t before = diags_.size();
    parseOperands(line, args, st.args);
    if (diags_.size() == before && static_cast<int>(st.args.size()) != info->operands) {
        synErr(line, std::string("a instrucao ") + info->mnemonic + " espera " +
                         std::to_string(info->operands) + " operando(s), mas foram encontrados " +
                         std::to_string(st.args.size()));
    }
    text_.push_back(st);
    textLoc_ += info->size;
}

void Assembler::handleDirective(int line, const std::string& label, bool hasLabel,
                                const std::string& op, const std::vector<Token>& args) {
    // ---- SECTION / SECAO ---------------------------------------------------
    if (op == "SECTION" || op == "SECAO") {
        requireNoLabel(line, hasLabel, op);
        if (args.size() != 1) {
            synErr(line, op + " espera exatamente 1 argumento (TEXT ou DATA)");
            return;
        }
        if (args[0].kind != Token::Kind::Ident || (args[0].text != "TEXT" && args[0].text != "DATA")) {
            synErr(line, "secao invalida '" + args[0].text + "': use TEXT ou DATA");
            return;
        }
        const Section target = args[0].text == "TEXT" ? Section::Text : Section::Data;
        bool& seen = target == Section::Text ? sawText_ : sawData_;
        if (seen) semErr(line, "secao " + args[0].text + " declarada mais de uma vez");
        seen = true;
        sawAnySection_ = true;
        current_ = target;
        return;
    }

    // ---- SPACE / CONST -----------------------------------------------------
    if (op == "SPACE" || op == "CONST") {
        if (current_ == Section::None) {
            semErr(line, "diretiva " + op + " fora de qualquer secao (use SECTION DATA)");
        } else if (current_ == Section::Text) {
            semErr(line, "diretiva " + op + " na secao TEXT (so permitida em DATA)");
        }
        bool ok = true;
        std::vector<long long> v;
        {
            // Commas are never valid here.
            std::vector<Token> clean;
            for (const auto& t : args) {
                if (t.kind == Token::Kind::Comma) {
                    synErr(line, "virgula inesperada em " + op);
                    ok = false;
                } else {
                    clean.push_back(t);
                }
            }
            bool vok = true;
            v = parseValues(line, clean, vok);
            ok = ok && vok;
        }

        Statement st;
        st.line = line;
        st.isSpace = (op == "SPACE");
        st.skipChecks = lineHasLexError_;

        if (op == "CONST") {
            if (v.size() != 1 && ok) synErr(line, "CONST espera exatamente 1 argumento (um numero decimal)");
            if (!v.empty()) st.number = v[0];
            if (v.size() == 1 && (st.number > INT_MAX || st.number < INT_MIN))
                semErr(line, "constante fora do intervalo de 32 bits");
        } else {
            if (v.size() > 1) synErr(line, "SPACE aceita no maximo 1 argumento (quantidade de palavras)");
            st.number = v.empty() ? 1 : v[0];
            if (v.size() == 1 && ok && (st.number <= 0 || st.number > 65536))
                semErr(line, "SPACE exige quantidade entre 1 e 65536");
            if (st.number <= 0 || st.number > 65536) st.number = 1;
        }

        if (hasLabel) {
            Symbol s;
            s.kind = st.isSpace ? SymKind::Space : SymKind::Const;
            s.section = Section::Data;
            s.address = dataLoc_;
            s.line = line;
            s.value = st.number;
            s.size = st.isSpace ? static_cast<int>(st.number) : 1;
            defineSymbol(label, s, line);
        }
        dataLoc_ += st.isSpace ? static_cast<int>(st.number) : 1;
        data_.push_back(st);
        return;
    }

    // ---- EQU -----------------------------------------------------------------
    if (op == "EQU") {
        if (!hasLabel) synErr(line, "EQU exige um rotulo (NOME: EQU valor)");
        if (sawAnySection_) semErr(line, "EQU deve ser declarado antes de qualquer secao");
        bool ok = true;
        const auto v = parseValues(line, args, ok);
        if (v.size() != 1) {
            if (ok) synErr(line, "EQU espera exatamente 1 argumento (um numero decimal)");
            return;
        }
        if (hasLabel) {
            Symbol s;
            s.kind = SymKind::Equ;
            s.line = line;
            s.value = v[0];
            defineSymbol(label, s, line);
        }
        return;
    }

    // ---- IF ------------------------------------------------------------------
    if (op == "IF") {
        requireNoLabel(line, hasLabel, op);
        if (current_ != Section::Text) semErr(line, "IF so e permitido dentro da secao TEXT");
        if (args.size() != 1) {
            synErr(line, "IF espera exatamente 1 argumento (um EQU)");
            return;
        }
        bool ok = true;
        const auto v = parseValues(line, args, ok);
        if (ok && v.size() == 1 && v[0] == 0) skipNext_ = true;  // false: drop the next line
        return;
    }

    // ---- BEGIN / END (modules) ---------------------------------------------
    if (op == "BEGIN") {
        std::string name = hasLabel ? label : "";
        if (!args.empty()) {
            if (!hasLabel && args.size() == 1 && args[0].kind == Token::Kind::Ident) {
                name = args[0].text;
            } else {
                synErr(line, "BEGIN nao aceita argumentos (use NOME: BEGIN)");
            }
        }
        if (hasBegin_) semErr(line, "BEGIN declarado mais de uma vez");
        hasBegin_ = true;
        beginLine_ = line;
        moduleName_ = name;
        return;
    }
    if (op == "END") {
        requireNoLabel(line, hasLabel, op);
        if (!args.empty()) synErr(line, "END nao aceita argumentos");
        if (hasEnd_) semErr(line, "END declarado mais de uma vez");
        hasEnd_ = true;
        endLine_ = line;
        return;
    }

    // ---- PUBLIC / EXTERN ---------------------------------------------------
    if (op == "PUBLIC") {
        requireNoLabel(line, hasLabel, op);
        if (args.size() != 1 || args[0].kind != Token::Kind::Ident) {
            synErr(line, "PUBLIC espera exatamente 1 argumento (o nome do simbolo)");
            return;
        }
        publics_.push_back({args[0].text, line});
        return;
    }
    if (op == "EXTERN") {
        std::string name = label;
        if (hasLabel && !args.empty()) {
            synErr(line, "EXTERN nao aceita argumentos quando usado como NOME: EXTERN");
            return;
        }
        if (!hasLabel) {
            if (args.size() != 1 || args[0].kind != Token::Kind::Ident) {
                synErr(line, "EXTERN exige um nome (NOME: EXTERN)");
                return;
            }
            name = args[0].text;
        }
        Symbol s;
        s.kind = SymKind::Extern;
        s.line = line;
        defineSymbol(name, s, line);
        return;
    }
}

void Assembler::parseLine(int lineNo, const std::string& text) {
    LexResult lx = tokenizeLine(text);
    lineHasLexError_ = !lx.errors.empty();
    for (const auto& e : lx.errors) lexErr(lineNo, e);

    const auto& tk = lx.tokens;
    if (tk.empty()) return;

    // Tolerated variant used by some course samples: "BEGIN: MODNAME".
    if (tk.size() == 3 && tk[0].kind == Token::Kind::Ident && tk[0].text == "BEGIN" &&
        tk[1].kind == Token::Kind::Colon && tk[2].kind == Token::Kind::Ident &&
        !findInstruction(tk[2].text) && !isDirective(tk[2].text)) {
        if (hasBegin_) semErr(lineNo, "BEGIN declarado mais de uma vez");
        hasBegin_ = true;
        beginLine_ = lineNo;
        moduleName_ = tk[2].text;
        return;
    }

    std::size_t i = 0;
    std::string label;
    bool hasLabel = false;

    if (tk[0].kind == Token::Kind::Colon) {
        synErr(lineNo, "':' sem rotulo antes");
        ++i;
    } else if (tk.size() >= 2 && tk[1].kind == Token::Kind::Colon) {
        if (tk[0].kind == Token::Kind::Ident) {
            label = tk[0].text;
            hasLabel = true;
        } else {
            lexErr(lineNo, "rotulo invalido '" + tk[0].text + "': nao pode comecar com numero");
        }
        i = 2;
        while (i + 1 < tk.size() && tk[i].kind == Token::Kind::Ident && tk[i + 1].kind == Token::Kind::Colon) {
            synErr(lineNo, "dois rotulos na mesma linha ('" + label + "' e '" + tk[i].text + "')");
            i += 2;
        }
    }

    // Label alone on a line: it labels the next statement.
    if (i >= tk.size()) {
        if (!hasLabel) return;
        if (skipNext_) return;  // the labelled line is being skipped by a false IF
        if (pending_.valid)
            synErr(lineNo, "dois rotulos seguidos para a mesma instrucao ('" + pending_.name + "' e '" + label + "')");
        pending_ = {label, lineNo, true};
        return;
    }

    if (pending_.valid) {
        if (hasLabel) {
            synErr(lineNo, "dois rotulos para a mesma instrucao ('" + pending_.name + "' e '" + label + "')");
        } else {
            label = pending_.name;
            hasLabel = true;
        }
        pending_.valid = false;
    }

    if (skipNext_) {  // false IF: this whole statement is discarded
        skipNext_ = false;
        return;
    }

    if (tk[i].kind != Token::Kind::Ident) {
        synErr(lineNo, "esperada uma instrucao ou diretiva, encontrado '" + tk[i].text + "'");
        return;
    }
    const std::string op = tk[i].text;
    const std::vector<Token> args(tk.begin() + static_cast<std::ptrdiff_t>(i) + 1, tk.end());

    if (const InstructionInfo* info = findInstruction(op)) {
        handleInstruction(lineNo, label, hasLabel, info, args);
    } else if (isDirective(op)) {
        handleDirective(lineNo, label, hasLabel, op, args);
    } else {
        synErr(lineNo, "instrucao ou diretiva invalida: '" + op + "'");
        if (hasLabel && current_ != Section::Data) {  // keep the label to avoid cascading errors
            Symbol s;
            s.kind = SymKind::Instruction;
            s.section = Section::Text;
            s.address = textLoc_;
            s.line = lineNo;
            defineSymbol(label, s, lineNo);
        }
    }
}

// ===========================================================================
// Pass 2
// ===========================================================================

void Assembler::checkStructure() {
    if (!sawText_) semErr(0, "secao TEXT ausente");
    if (hasBegin_ && !hasEnd_) semErr(beginLine_, "BEGIN sem o END correspondente");
    if (hasEnd_ && !hasBegin_) semErr(endLine_, "END sem o BEGIN correspondente");
}

void Assembler::emitOperand(const Statement& st, std::size_t index, Role role, bool isDiv) {
    const Arg& arg = st.args[index];
    if (st.skipChecks || !arg.valid) {
        push(0, 0);
        return;
    }
    auto it = symbols_.find(arg.name);
    if (it == symbols_.end()) {
        semErr(st.line, "rotulo '" + arg.name + "' nao declarado");
        push(0, 0);
        return;
    }
    const Symbol& s = it->second;

    if (s.kind == SymKind::Equ) {
        semErr(st.line, "'" + arg.name + "' foi definido com EQU e nao pode ser operando de instrucao");
        push(0, 0);
        return;
    }
    if (role == Role::Jump) {
        if (s.kind == SymKind::Const || s.kind == SymKind::Space)
            semErr(st.line, "salto para o rotulo de dados '" + arg.name + "' (esperado rotulo de codigo)");
    } else {
        if (s.kind == SymKind::Instruction)
            semErr(st.line, "operando '" + arg.name + "' e um rotulo de codigo (esperado rotulo de dados)");
        if (role == Role::Write && s.kind == SymKind::Const)
            semErr(st.line, "modificacao de valor constante: '" + arg.name + "' foi declarado com CONST");
    }
    if (isDiv && s.kind == SymKind::Const && s.value == 0 && arg.offset == 0)
        semErr(st.line, "divisao por zero: '" + arg.name + "' e uma constante igual a 0");

    if (s.kind == SymKind::Extern) {
        uses_.push_back({arg.name, static_cast<int>(textWords_.size())});
        push(arg.offset, 0);  // the linker adds the symbol address to this addend
    } else {
        push(finalAddress(s) + arg.offset, 1);
    }
}

void Assembler::generate() {
    inText_ = true;
    for (const auto& st : text_) {
        push(st.info->opcode, 0);
        for (std::size_t k = 0; k < static_cast<std::size_t>(st.info->operands); ++k) {
            if (k < st.args.size())
                emitOperand(st, k, roleFor(*st.info, k), st.info->opcode == 4);
            else
                push(0, 0);  // operand missing (already reported): keep the size consistent
        }
    }
    inText_ = false;
    for (const auto& st : data_) {
        if (st.isSpace) {
            for (long long n = 0; n < st.number; ++n) push(0, 0);
        } else {
            push(st.number, 0);
        }
    }
}

AssemblyResult Assembler::run() {
    for (std::size_t i = 0; i < lines_.size(); ++i) parseLine(static_cast<int>(i) + 1, lines_[i]);
    lineHasLexError_ = false;
    if (pending_.valid) synErr(pending_.line, "rotulo '" + pending_.name + "' sem instrucao ou diretiva associada");

    checkStructure();
    generate();

    AssemblyResult result;
    for (const auto& p : publics_) {
        auto it = symbols_.find(p.name);
        if (it == symbols_.end()) {
            semErr(p.line, "simbolo PUBLIC '" + p.name + "' nao foi definido neste modulo");
        } else if (it->second.kind == SymKind::Extern) {
            semErr(p.line, "simbolo '" + p.name + "' nao pode ser PUBLIC e EXTERN ao mesmo tempo");
        } else if (it->second.kind == SymKind::Equ) {
            semErr(p.line, "simbolo EQU '" + p.name + "' nao pode ser PUBLIC");
        } else {
            const bool dup = std::any_of(result.definitions.begin(), result.definitions.end(),
                                         [&](const Definition& d) { return d.name == p.name; });
            if (!dup) result.definitions.push_back({p.name, finalAddress(it->second)});
        }
    }

    // Errors are listed in source order; whole-file errors (line 0) go last.
    std::stable_sort(diags_.begin(), diags_.end(), [](const Diagnostic& a, const Diagnostic& b) {
        const int la = a.line == 0 ? INT_MAX : a.line;
        const int lb = b.line == 0 ? INT_MAX : b.line;
        return la < lb;
    });

    result.diagnostics = diags_;
    result.words = textWords_;
    result.words.insert(result.words.end(), dataWords_.begin(), dataWords_.end());
    result.relocation = textReloc_;
    result.relocation.insert(result.relocation.end(), dataReloc_.begin(), dataReloc_.end());
    result.uses = uses_;
    result.moduleName = moduleName_;
    result.isModule = hasBegin_;
    return result;
}

}  // namespace

AssemblyResult assemble(const std::vector<std::string>& sourceLines) {
    Assembler a(sourceLines);
    return a.run();
}

}  // namespace sb
