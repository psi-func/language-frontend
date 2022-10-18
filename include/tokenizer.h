#pragma once

#include <string>

extern std::string IdentifierStr; // filled oin if tok_identifier
extern double NumVal;             // Filled in if tok_number

// Lexer returns tokens [0-255] if it is an unknown char, otherwise 1
enum Token
{
    tok_eof = -1,

    // commands
    tok_def = -2,
    tok_extern = -3,

    // primary
    tok_identifier = -4,
    tok_number = -5,
};

int gettok();