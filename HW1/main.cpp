#include "tokens.hpp"
#include "output.hpp"
#include <iostream>
#include <string>

std::string curr = "";
bool EOS = false;

void concat(const char *str){
    if(EOS)
        return;
    curr.append(str);
}

void escapeCheck(const char *str){
    if(EOS)
        return;

    std::string temp = str;
    if(temp == "\\0"){
        curr.append(temp);
        EOS = true;
    } else if(temp.substr(0,2) == "\\x"){
        int hex = std::stoi(temp.substr(2), nullptr, 16);
        curr.append(1, static_cast<char>(hex));
    } else{
        char c = temp=="\\n" ? '\n' :
                 temp=="\\t" ? '\t' :
                 temp=="\\r" ? '\r' :
                 temp=="\\\\" ? '\\' :'\"';
        curr.append(1, c);
    }
}

int main() {
    enum tokentype token;

    // read tokens until the end of file is reached
    while ((token = static_cast<tokentype>(yylex()))) {
        if(token == tokentype::STRING){
            output::printToken(yylineno, token, curr.c_str());
            curr = "";
            EOS = false;
        }
    }
    return 0;
}