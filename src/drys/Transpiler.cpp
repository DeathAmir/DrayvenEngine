#include "drayven/Drys.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace drayven::drys {
namespace {

std::string trim(std::string s) {
    auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return {};
    auto b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

bool starts(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }

std::string sanitize(std::string s) {
    for (char& c : s) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') c = '_';
    if (s.empty() || std::isdigit(static_cast<unsigned char>(s[0]))) s = "_" + s;
    return s;
}

std::uint64_t mix64(std::uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31; return x;
}

std::uint64_t hashName(std::string_view s, std::uint64_t seed) {
    std::uint64_t h = 1469598103934665603ULL ^ seed;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return mix64(h);
}

std::string mangled(std::string_view name, std::uint64_t seed) {
    const auto a = hashName(name, seed);
    const auto b = mix64(a ^ 0xD2B74407B1CE6E93ULL);
    std::ostringstream out;
    out << "_dv_" << std::hex << std::setfill('0') << std::setw(16) << a << std::setw(16) << b;
    return out.str();
}

std::string replaceSymbols(const std::string& input, const std::unordered_map<std::string, std::string>& symbols) {
    std::string out;
    out.reserve(input.size() + 32);
    bool stringMode = false;
    char quote = 0;
    for (std::size_t i = 0; i < input.size();) {
        const char c = input[i];
        if (stringMode) {
            out.push_back(c);
            if (c == '\\' && i + 1 < input.size()) { out.push_back(input[++i]); ++i; continue; }
            if (c == quote) stringMode = false;
            ++i;
            continue;
        }
        if (c == '"' || c == '\'') {
            stringMode = true; quote = c; out.push_back(c); ++i; continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t j = i + 1;
            while (j < input.size()) {
                char d = input[j];
                if (!std::isalnum(static_cast<unsigned char>(d)) && d != '_') break;
                ++j;
            }
            std::string word = input.substr(i, j - i);
            auto it = symbols.find(word);
            out += it == symbols.end() ? word : it->second;
            i = j;
            continue;
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

std::string cppExpr(std::string s, const std::unordered_map<std::string, std::string>& symbols) {
    static const std::unordered_map<std::string, std::string> calls = {
        {"log(", "drayven::script::log("}, {"key_down(", "drayven::script::keyDown("},
        {"key_pressed(", "drayven::script::keyPressed("}, {"move_x(", "drayven::script::moveX("},
        {"move_y(", "drayven::script::moveY("}, {"set_position(", "drayven::script::setPosition("},
        {"play_sound(", "drayven::script::playSound("}, {"spawn(", "drayven::script::spawn("},
        {"destroy(", "drayven::script::destroy("}, {"delta_time(", "drayven::script::deltaTime("},
        {"rand(", "drayven::script::random("}
    };
    for (const auto& [a, b] : calls) {
        std::size_t p = 0;
        while ((p = s.find(a, p)) != std::string::npos) { s.replace(p, a.size(), b); p += b.size(); }
    }
    std::size_t p = 0;
    while ((p = s.find(" and ", p)) != std::string::npos) s.replace(p, 5, " && ");
    p = 0;
    while ((p = s.find(" or ", p)) != std::string::npos) s.replace(p, 4, " || ");
    if (starts(s, "not ")) s = "!" + s.substr(4);
    return replaceSymbols(s, symbols);
}

std::string luaExpr(std::string s, const std::unordered_map<std::string, std::string>& symbols) {
    static const std::unordered_map<std::string, std::string> calls = {
        {"log(", "drayven.log("}, {"key_down(", "drayven.key_down("},
        {"key_pressed(", "drayven.key_pressed("}, {"move_x(", "drayven.move_x("},
        {"move_y(", "drayven.move_y("}, {"set_position(", "drayven.set_position("},
        {"play_sound(", "drayven.play_sound("}, {"spawn(", "drayven.spawn("},
        {"destroy(", "drayven.destroy("}, {"delta_time(", "drayven.delta_time("},
        {"rand(", "drayven.rand("}
    };
    for (const auto& [a, b] : calls) {
        std::size_t p = 0;
        while ((p = s.find(a, p)) != std::string::npos) { s.replace(p, a.size(), b); p += b.size(); }
    }
    return replaceSymbols(s, symbols);
}

std::string symbolFor(std::string raw, const TranspileOptions& o,
                      std::unordered_map<std::string, std::string>& symbols) {
    raw = sanitize(raw);
    auto it = symbols.find(raw);
    if (it != symbols.end()) return it->second;
    auto generated = o.hardenSymbols ? mangled(raw, o.seed ^ symbols.size()) : raw;
    symbols.emplace(raw, generated);
    return generated;
}

std::vector<std::string> splitArgs(std::string args) {
    std::stringstream ss(args);
    std::string a;
    std::vector<std::string> out;
    while (std::getline(ss, a, ',')) {
        a = trim(a);
        if (!a.empty()) out.push_back(a);
    }
    return out;
}

}

TranspileResult Transpiler::transpile(std::string_view source, std::string_view moduleName,
                                      const TranspileOptions& options) const {
    TranspileResult r;
    std::stringstream in{std::string(source)};
    std::string line;
    int lineNo = 0;
    bool inFn = false;
    int cppIndent = 1;
    std::vector<std::string> blockStack;
    std::unordered_map<std::string, std::string> symbols;
    std::string game = sanitize(std::string(moduleName));

    std::ostringstream globals;
    std::ostringstream body;
    std::ostringstream lua;
    if (options.language == OutputLanguage::Lua) {
        lua << "-- Generated by Drayven Script (DRYS) 0.2. Source-free build artifact.\n"
            << "local M = {}\n";
    }

    while (std::getline(in, line)) {
        ++lineNo;
        auto t = trim(line);
        if (t.empty()) continue;
        if (starts(t, "#") || starts(t, "//")) {
            if (!options.stripComments && options.language == OutputLanguage::Lua) lua << "-- " << t << "\n";
            continue;
        }
        if (starts(t, "game ")) {
            game = sanitize(trim(t.substr(5)));
            continue;
        }
        if (starts(t, "import ")) {
            if (options.language == OutputLanguage::Lua) {
                auto mod = trim(t.substr(7));
                lua << "local " << sanitize(mod) << " = require(\"" << mod << "\")\n";
            }
            continue;
        }

        if (starts(t, "var ") || starts(t, "let ")) {
            auto rest = trim(t.substr(4));
            auto eq = rest.find('=');
            auto rawName = trim(eq == std::string::npos ? rest : rest.substr(0, eq));
            auto name = symbolFor(rawName, options, symbols);
            auto val = eq == std::string::npos ? "0" : trim(rest.substr(eq + 1));
            if (options.language == OutputLanguage::Cpp) {
                val = cppExpr(val, symbols);
                if (!inFn) globals << "    drayven::script::Value " << name << " = " << val << ";\n";
                else body << std::string(cppIndent * 4, ' ') << "auto " << name << " = " << val << ";\n";
            } else {
                val = luaExpr(val, symbols);
                lua << (inFn ? "    local " : "local ") << name << " = " << val << "\n";
            }
            continue;
        }

        if (starts(t, "fn ")) {
            auto sig = trim(t.substr(3));
            auto p = sig.find('(');
            auto q = sig.rfind(')');
            if (p == std::string::npos || q == std::string::npos || q < p) {
                r.ok = false;
                r.diagnostics.push_back("line " + std::to_string(lineNo) + ": invalid function declaration");
                continue;
            }
            auto rawName = trim(sig.substr(0, p));
            auto name = symbolFor(rawName, options, symbols);
            auto rawArgs = splitArgs(sig.substr(p + 1, q - p - 1));
            std::vector<std::string> args;
            for (auto& a : rawArgs) args.push_back(symbolFor(a, options, symbols));

            if (options.language == OutputLanguage::Cpp) {
                body << "    void " << name << "(";
                for (std::size_t i = 0; i < args.size(); ++i) {
                    if (i) body << ", ";
                    body << "double " << args[i];
                }
                body << ") {\n";
                cppIndent = 2;
            } else {
                lua << "function M." << name << "(";
                for (std::size_t i = 0; i < args.size(); ++i) {
                    if (i) lua << ", ";
                    lua << args[i];
                }
                lua << ")\n";
            }
            blockStack.push_back("fn");
            inFn = true;
            continue;
        }

        if (t == "end") {
            if (blockStack.empty()) {
                r.ok = false;
                r.diagnostics.push_back("line " + std::to_string(lineNo) + ": unexpected end");
                continue;
            }
            auto block = blockStack.back();
            blockStack.pop_back();
            if (options.language == OutputLanguage::Cpp) {
                --cppIndent;
                body << std::string(cppIndent * 4, ' ') << "}\n";
            } else {
                lua << "end\n";
            }
            if (block == "fn") inFn = false;
            continue;
        }

        if (starts(t, "if ")) {
            auto condition = trim(t.substr(3));
            if (options.language == OutputLanguage::Cpp) {
                body << std::string(cppIndent * 4, ' ') << "if (" << cppExpr(condition, symbols) << ") {\n";
                ++cppIndent;
            } else {
                lua << "    if " << luaExpr(condition, symbols) << " then\n";
            }
            blockStack.push_back("if");
            continue;
        }

        if (t == "else") {
            if (blockStack.empty() || blockStack.back() != "if") {
                r.ok = false;
                r.diagnostics.push_back("line " + std::to_string(lineNo) + ": else without if");
                continue;
            }
            if (options.language == OutputLanguage::Cpp) {
                --cppIndent;
                body << std::string(cppIndent * 4, ' ') << "} else {\n";
                ++cppIndent;
            } else {
                lua << "    else\n";
            }
            continue;
        }

        if (starts(t, "while ")) {
            auto condition = trim(t.substr(6));
            if (options.language == OutputLanguage::Cpp) {
                body << std::string(cppIndent * 4, ' ') << "while (" << cppExpr(condition, symbols) << ") {\n";
                ++cppIndent;
            } else {
                lua << "    while " << luaExpr(condition, symbols) << " do\n";
            }
            blockStack.push_back("while");
            continue;
        }

        if (starts(t, "return")) {
            auto rest = trim(t.substr(6));
            if (options.language == OutputLanguage::Cpp) {
                body << std::string(cppIndent * 4, ' ') << "return";
                if (!rest.empty()) body << " " << cppExpr(rest, symbols);
                body << ";\n";
            } else {
                lua << "    return";
                if (!rest.empty()) lua << " " << luaExpr(rest, symbols);
                lua << "\n";
            }
            continue;
        }

        if (options.language == OutputLanguage::Cpp) {
            auto st = cppExpr(t, symbols);
            if (!st.empty() && st.back() != ';') st += ';';
            body << std::string(cppIndent * 4, ' ') << st << "\n";
        } else {
            lua << "    " << luaExpr(t, symbols) << "\n";
        }
    }

    if (!blockStack.empty()) {
        r.ok = false;
        r.diagnostics.push_back("unclosed block at end of file");
    }

    if (options.language == OutputLanguage::Cpp) {
        std::ostringstream out;
        out << "// Generated by Drayven Script (DRYS) 0.2. Do not edit.\n"
            << "#include <drayven/ScriptAPI.hpp>\n\nnamespace drayven::generated {\n"
            << "class " << (options.hardenSymbols ? mangled(game, options.seed ^ 0xA17EULL) : game) << " final {\npublic:\n"
            << globals.str() << body.str() << "};\n}\n";
        r.cpp = out.str();
        r.output = r.cpp;
    } else {
        lua << "return M\n";
        r.lua = lua.str();
        r.output = r.lua;
    }
    return r;
}

TranspileResult Transpiler::transpileFile(const std::filesystem::path& source,
                                          const TranspileOptions& options) const {
    std::ifstream in(source, std::ios::binary);
    if (!in) return {false, "", "", "", {"cannot open " + source.string()}};
    std::stringstream ss;
    ss << in.rdbuf();
    return transpile(ss.str(), source.stem().string(), options);
}
}
