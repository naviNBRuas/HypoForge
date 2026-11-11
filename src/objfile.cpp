#include "objfile.hpp"

#include <fstream>
#include <sstream>

namespace sb {

std::string formatPlainObject(const std::vector<long long>& words) {
    // Same layout as the reference object files shipped with the assignment:
    // every word followed by one blank, no trailing newline.
    std::string out;
    for (long long w : words) out += std::to_string(w) + " ";
    return out;
}

std::string formatModuleObject(const AssemblyResult& r) {
    std::string out;
    for (const auto& d : r.definitions) out += "D, " + d.name + " " + std::to_string(d.address) + "\n";
    for (const auto& u : r.uses) out += "U, " + u.name + " " + std::to_string(u.address) + "\n";
    out += "R,";
    for (int bit : r.relocation) out += " " + std::to_string(bit);
    out += "\n";
    out += formatPlainObject(r.words);
    return out;
}

namespace {
std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    return s.substr(b, s.find_last_not_of(ws) - b + 1);
}
}  // namespace

bool parseModuleObject(const std::string& text, ModuleObject& out, std::string& error) {
    std::istringstream in(text);
    std::string raw;
    bool haveCode = false, haveReloc = false;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        const std::string line = trim(raw);
        if (line.empty()) continue;
        std::istringstream ls(line);
        if (line.rfind("D,", 0) == 0 || line.rfind("U,", 0) == 0) {
            std::string tag, name;
            int addr = 0;
            ls >> tag >> name;
            if (!(ls >> addr)) {
                error = "linha " + std::to_string(lineNo) + ": entrada de tabela malformada";
                return false;
            }
            if (line[0] == 'D') out.definitions.push_back({name, addr});
            else out.uses.push_back({name, addr});
        } else if (line.rfind("R,", 0) == 0) {
            std::string tag;
            ls >> tag;
            int bit;
            while (ls >> bit) out.relocation.push_back(bit);
            haveReloc = true;
        } else {
            if (haveCode) {
                error = "linha " + std::to_string(lineNo) + ": codigo objeto deve ocupar uma unica linha";
                return false;
            }
            long long w;
            while (ls >> w) out.code.push_back(w);
            if (!ls.eof()) {
                error = "linha " + std::to_string(lineNo) + ": palavra invalida no codigo objeto";
                return false;
            }
            haveCode = true;
        }
    }
    if (!haveCode) { error = "arquivo objeto sem linha de codigo"; return false; }
    if (!haveReloc) { error = "arquivo objeto sem tabela de realocacao (linha R)"; return false; }
    if (out.relocation.size() != out.code.size()) {
        error = "tabela de realocacao (" + std::to_string(out.relocation.size()) +
                " entradas) incompativel com o codigo (" + std::to_string(out.code.size()) + " palavras)";
        return false;
    }
    return true;
}

bool readWholeFile(const std::string& path, std::string& content) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    content = ss.str();
    return true;
}

bool writeWholeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << content;
    return static_cast<bool>(f);
}

bool readLines(const std::string& path, std::vector<std::string>& lines) {
    std::string content;
    if (!readWholeFile(path, content)) return false;
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) content.erase(0, 3);
    std::string cur;
    for (std::size_t i = 0; i < content.size(); ++i) {
        const char c = content[i];
        if (c == '\r') {
            if (i + 1 < content.size() && content[i + 1] == '\n') ++i;  // CRLF
            lines.push_back(cur);
            cur.clear();
        } else if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) lines.push_back(cur);
    return true;
}

std::string replaceExtension(const std::string& path, const std::string& newExt) {
    const auto slash = path.find_last_of("/\\");
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return path + newExt;
    return path.substr(0, dot) + newExt;
}

}  // namespace sb
