#include "AST.h"
#include "tokenizer.h"

#include <cstdio>
#include <memory>
#include <cctype>
#include <cstdio>

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;

static int getNextToken()
{
    return CurTok = gettok();
}

/// BinopPrecendence - This holds the precendence for each binary operator that is defined
std::map<char, int> BinopPrecendence;

/// GetTokPrecedence - Get the precendence of the pending binary operator token
static int GetTokPrecedence()
{
    if (!__isascii(CurTok))
        return -1;

    /// Make sure it's a declared binop.
    auto TokPrec = BinopPrecendence.find(CurTok);
    if (TokPrec != BinopPrecendence.end())
        return TokPrec->second;
    else
        return -1;
}

/// LogError* - these are little helper functions for error handling
std::unique_ptr<ExprAST> LogError(const char *Str)
{
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

/// parenexpr ::= '(' expr ')'
static std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ).
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string IdName = IdentifierStr;

    getNextToken(); // eat identifier.

    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    // call
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')')
    {
        while (1)
        {
            if (auto Arg = ParseExpression())
            {
                Args.push_back(std::move(Arg));
            }
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' ot ',' in argument list");
            getNextToken();
        }
    }
    // eat ')'
    getNextToken();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///     ::= identifierexpr
///     ::= numberexpr
///     ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (CurTok)
    {
    default:
        return LogError("unknown token expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

/// binoprhs
///     ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS)
{
    while(1) {
        int TokPrec = GetTokPrecedence();

        if (TokPrec < ExprPrec)
            return LHS;
        int BinOp = CurTok;
        getNextToken(); // eat binop;

        // Parse primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS) return nullptr;

        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
            if (!RHS) 
                return nullptr; 
        }

        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));

    }
}

/// expr
///     ::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression()
{
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///     ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function in prototype");
    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");
    
    std::vector<std::string> ArgNames;
    while(getNextToken() == tok_identifier) {
        ArgNames.push_back(IdentifierStr);
    }
    if (CurTok != ')') 
        return LogErrorP("Expected ')' in prototype");

    getNextToken(); // eat ')'
    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames)); 
}

/// definition ::= 'def' prototype expr
static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat 'def'.
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression()) 
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));

    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // eat 'extern'
    return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

// TOP LEVEL

static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (ParseExtern()) {
    fprintf(stderr, "Parsed an extern\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


/// top ::= definition | external | expression | ';'
void MainLoop() {
    while(1) {
        fprintf(stderr, "ready> ");
        getNextToken();
        
        switch (CurTok)
        {
        case tok_eof:
            return;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        case ';':
            getNextToken();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}