#include "drayven/EditorApp.hpp"
#include <unordered_map>
#include <vector>

namespace drayven {
struct Shape { char32_t iso, fin, ini, med; bool joinPrev, joinNext; };
static const std::unordered_map<char32_t,Shape> shapes={
{U'ا',{0xFE8D,0xFE8E,0,0,true,false}},{U'آ',{0xFE81,0xFE82,0,0,true,false}},{U'ب',{0xFE8F,0xFE90,0xFE91,0xFE92,true,true}},
{U'پ',{0xFB56,0xFB57,0xFB58,0xFB59,true,true}},{U'ت',{0xFE95,0xFE96,0xFE97,0xFE98,true,true}},{U'ث',{0xFE99,0xFE9A,0xFE9B,0xFE9C,true,true}},
{U'ج',{0xFE9D,0xFE9E,0xFE9F,0xFEA0,true,true}},{U'چ',{0xFB7A,0xFB7B,0xFB7C,0xFB7D,true,true}},{U'ح',{0xFEA1,0xFEA2,0xFEA3,0xFEA4,true,true}},
{U'خ',{0xFEA5,0xFEA6,0xFEA7,0xFEA8,true,true}},{U'د',{0xFEA9,0xFEAA,0,0,true,false}},{U'ذ',{0xFEAB,0xFEAC,0,0,true,false}},
{U'ر',{0xFEAD,0xFEAE,0,0,true,false}},{U'ز',{0xFEAF,0xFEB0,0,0,true,false}},{U'ژ',{0xFB8A,0xFB8B,0,0,true,false}},
{U'س',{0xFEB1,0xFEB2,0xFEB3,0xFEB4,true,true}},{U'ش',{0xFEB5,0xFEB6,0xFEB7,0xFEB8,true,true}},{U'ص',{0xFEB9,0xFEBA,0xFEBB,0xFEBC,true,true}},
{U'ض',{0xFEBD,0xFEBE,0xFEBF,0xFEC0,true,true}},{U'ط',{0xFEC1,0xFEC2,0xFEC3,0xFEC4,true,true}},{U'ظ',{0xFEC5,0xFEC6,0xFEC7,0xFEC8,true,true}},
{U'ع',{0xFEC9,0xFECA,0xFECB,0xFECC,true,true}},{U'غ',{0xFECD,0xFECE,0xFECF,0xFED0,true,true}},{U'ف',{0xFED1,0xFED2,0xFED3,0xFED4,true,true}},
{U'ق',{0xFED5,0xFED6,0xFED7,0xFED8,true,true}},{U'ک',{0xFB8E,0xFB8F,0xFB90,0xFB91,true,true}},{U'گ',{0xFB92,0xFB93,0xFB94,0xFB95,true,true}},
{U'ل',{0xFEDD,0xFEDE,0xFEDF,0xFEE0,true,true}},{U'م',{0xFEE1,0xFEE2,0xFEE3,0xFEE4,true,true}},{U'ن',{0xFEE5,0xFEE6,0xFEE7,0xFEE8,true,true}},
{U'و',{0xFEED,0xFEEE,0,0,true,false}},{U'ه',{0xFEE9,0xFEEA,0xFEEB,0xFEEC,true,true}},{U'ی',{0xFBFC,0xFBFD,0xFBFE,0xFBFF,true,true}}
};
static std::vector<char32_t> decode(std::string_view s){std::vector<char32_t> o;for(size_t i=0;i<s.size();){unsigned char c=s[i];char32_t cp=0;size_t n=1;if(c<0x80)cp=c;else if((c&0xE0)==0xC0){cp=c&0x1F;n=2;}else if((c&0xF0)==0xE0){cp=c&0x0F;n=3;}else{cp=c&0x07;n=4;}for(size_t j=1;j<n&&i+j<s.size();++j)cp=(cp<<6)|(s[i+j]&0x3F);o.push_back(cp);i+=n;}return o;}
static void enc(std::string& o,char32_t c){if(c<0x80)o.push_back((char)c);else if(c<0x800){o.push_back((char)(0xC0|(c>>6)));o.push_back((char)(0x80|(c&63)));}else if(c<0x10000){o.push_back((char)(0xE0|(c>>12)));o.push_back((char)(0x80|((c>>6)&63)));o.push_back((char)(0x80|(c&63)));}else{o.push_back((char)(0xF0|(c>>18)));o.push_back((char)(0x80|((c>>12)&63)));o.push_back((char)(0x80|((c>>6)&63)));o.push_back((char)(0x80|(c&63)));}}
std::string persianVisualOrder(std::string_view utf8){auto c=decode(utf8);std::vector<char32_t>s=c;for(size_t i=0;i<c.size();++i){auto it=shapes.find(c[i]);if(it==shapes.end())continue;bool p=false,n=false;if(i>0){auto q=shapes.find(c[i-1]);p=q!=shapes.end()&&q->second.joinNext&&it->second.joinPrev;}if(i+1<c.size()){auto q=shapes.find(c[i+1]);n=q!=shapes.end()&&it->second.joinNext&&q->second.joinPrev;}auto sh=it->second;s[i]=(p&&n&&sh.med)?sh.med:(p&&sh.fin)?sh.fin:(n&&sh.ini)?sh.ini:sh.iso;}std::string out;for(auto it=s.rbegin();it!=s.rend();++it)enc(out,*it);return out;}
}
