#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source){
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAlpha(char c){
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isDigit(char c){
    return c >= '0' && c <= '9';
}


// If the current character is a null byte, then we have reached the end.
static bool isAtEnd(){
    return *scanner.current == '\0';
}

// Consumes the current character and returns it
static char advance(){
    scanner.current++;
    return scanner.current[-1];
}

// Do not consume any non-whitespace characters
static char peek() {
    return *scanner.current;
}

// Lookahead function that looks one character beyond the current one without consuming it
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}

// Conditionally consumes the econd character
static bool match(char expected){
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

// To create a token, we have this constructor like function
// It uses the scanner's start and current pointers to capture tthe token's lexeme
static Token makeToken(TokenType type){
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

// Function for returning error token, only difference is that the 'lexeme' points to the
// error message string instead of the user's source code 
static Token errorToken(const char* message){
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

// Skips whitespace and advances the scanner past any leading whitespace. After this call returns
// we know the very next character is a meaningful one (or we're at the end of the file)
static void skipWhitespace() {
    for(;;){
        char c = peek();
        switch(c){
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek() == '/'){
                    // A comment goes until the end of the line
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }

    }
}

// Check the rest of the letters
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type){
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0){
        return type;
    }

    return TOKEN_IDENTIFIER;
}


//  handle the easy keywords
static TokenType identifierType(){
    switch (scanner.start[0]){
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f': 
            if (scanner.current - scanner.start > 1){
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 2, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1){
                switch (scanner.start[1]){
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

// Scan an identifier once found
static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

static Token number(){
    while (isDigit(peek())) advance();

    // look for a fractional part
    if (peek() == '.' && isDigit(peekNext())){
        // consume the "."
        advance();

        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

// We consume characters until we reach the closing quote. We also track newlines inside the 
// string literal (Lox supports multi strings)
static Token string(){
    while (peek() != '"' && !isAtEnd()){
        if (peek() == '\n') scanner.line++;
        advance();
    }
        if (isAtEnd()) return errorToken("Unterminated string.");

        // The closing quote
        advance();
        return makeToken(TOKEN_STRING);
    
}

// Since each call to this function scans a complete token, we know we are at the 
// beginning of a new token when we enter the function
Token scanToken(){
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd()) return makeToken(TOKEN_EOF);

    char c = advance();

    if (isAlpha(c)) return identifier();

    if (isDigit(c)) return number();

    switch(c){
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
    }

    return errorToken("Unexpected character.");
}

