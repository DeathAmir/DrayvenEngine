#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace drayven::drys {
enum class TokenKind { Identifier, Number, String, Keyword, Symbol, Newline, End };
struct Token { TokenKind kind; std::string text; std::size_t line; std::size_t column; };
class Lexer { public: explicit Lexer(std::string_view source):m_source(source){} std::vector<Token> run(); private: std::string_view m_source; };
enum class OutputLanguage { Cpp, Lua };
struct TranspileOptions { OutputLanguage language{OutputLanguage::Cpp}; bool hardenSymbols{false}; std::uint64_t seed{0x4452415956454eULL}; bool stripComments{true}; };
struct TranspileResult { bool ok{true}; std::string output, cpp, lua; std::vector<std::string> diagnostics; };
class Transpiler {
public:
 TranspileResult transpile(std::string_view source,std::string_view moduleName="GameScript",const TranspileOptions& options={}) const;
 TranspileResult transpileFile(const std::filesystem::path& source,const TranspileOptions& options={}) const;
};
}
