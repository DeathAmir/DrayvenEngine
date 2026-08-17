#include "drayven/Drys.hpp"
#include <cctype>
#include <unordered_set>

namespace drayven::drys {
std::vector<Token> Lexer::run(){
    static const std::unordered_set<std::string> kw={"game","fn","end","var","let","if","else","while","return","true","false","on","component","import"};
    std::vector<Token> out; std::size_t i=0,line=1,col=1;
    auto push=[&](TokenKind k,std::string s,std::size_t l,std::size_t c){out.push_back({k,std::move(s),l,c});};
    while(i<m_source.size()){
        char c=m_source[i];
        if(c=='\r'){++i;continue;} if(c=='\n'){push(TokenKind::Newline,"\n",line,col);++i;++line;col=1;continue;}
        if(std::isspace((unsigned char)c)){++i;++col;continue;}
        if(c=='#' || (c=='/' && i+1<m_source.size() && m_source[i+1]=='/')){while(i<m_source.size()&&m_source[i]!='\n'){++i;++col;}continue;}
        if(std::isalpha((unsigned char)c)||c=='_'){auto l=line,cc=col,s=i;while(i<m_source.size()&&(std::isalnum((unsigned char)m_source[i])||m_source[i]=='_')){++i;++col;}std::string t(m_source.substr(s,i-s));push(kw.contains(t)?TokenKind::Keyword:TokenKind::Identifier,std::move(t),l,cc);continue;}
        if(std::isdigit((unsigned char)c)||(c=='.'&&i+1<m_source.size()&&std::isdigit((unsigned char)m_source[i+1]))){auto l=line,cc=col,s=i;bool dot=false;while(i<m_source.size()&&(std::isdigit((unsigned char)m_source[i])||(!dot&&m_source[i]=='.'))){if(m_source[i]=='.')dot=true;++i;++col;}push(TokenKind::Number,std::string(m_source.substr(s,i-s)),l,cc);continue;}
        if(c=='"'){auto l=line,cc=col,s=i++;++col;bool esc=false;while(i<m_source.size()){char q=m_source[i++];++col;if(q=='"'&&!esc)break;esc=(q=='\\'&&!esc);if(q!='\\')esc=false;}push(TokenKind::String,std::string(m_source.substr(s,i-s)),l,cc);continue;}
        auto l=line,cc=col; std::string sym(1,c); if(i+1<m_source.size()){std::string two(m_source.substr(i,2)); if(two=="=="||two=="!="||two==">="||two=="<="||two=="&&"||two=="||"||two=="+="||two=="-="||two=="->"){sym=two;i+=2;col+=2;push(TokenKind::Symbol,sym,l,cc);continue;}}
        ++i;++col;push(TokenKind::Symbol,sym,l,cc);
    }
    out.push_back({TokenKind::End,"",line,col}); return out;
}
}
