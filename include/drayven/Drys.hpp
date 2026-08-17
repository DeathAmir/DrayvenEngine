#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace drayven::drys {
enum class TokenKind { Identifier, Number, String, Keyword, Symbol, Newline, End };
struct Token { TokenKind kind; std::string text; std::size_t line; std::size_t column; };

class Lexer {
public:
    explicit Lexer(std::string_view source): m_source(source) {}
    std::vector<Token> run();
private:
    std::string_view m_source;
};

struct TranspileResult { bool ok{true}; std::string cpp; std::vector<std::string> diagnostics; };
class Transpiler {
public:
    TranspileResult transpile(std::string_view source, std::string_view moduleName = "GameScript") const;
    TranspileResult transpileFile(const std::filesystem::path& source) const;
};
}
