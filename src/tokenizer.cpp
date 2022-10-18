#include "tokenizer.h"

#include <cctype>
#include <cstdio>
#include <string>

std::string IdentifierStr; // filled oin if tok_identifier
double NumVal;             // Filled in if tok_number

int gettok()
{
    static int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) // Identifier [a-zA-Z][a-zA-Z0-9]
    { 
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }
    if (isdigit(LastChar) || LastChar == '.') // number: [0-9.]+
    {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }
    if (LastChar == '#') {
        // comment until end of line.
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }
    if (LastChar == EOF)
        return tok_eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}