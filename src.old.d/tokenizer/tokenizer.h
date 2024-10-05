#ifndef TOKENIZER_H
#define TOKENIZER_H

#define MAX_TOKENS 10000

#define MAX_TOK_LEN 256

typedef enum TokenType {
    TOK_NONE = 0,
    TOK_VARIABLE,
    TOK_OPEN_BRACKET,
    TOK_CLOSE_BRACKET,
    TOK_VALUE,
} TokenType;

typedef struct Token {
    int index;
    int len;
    TokenType type;
    char val[MAX_TOK_LEN];
} Token;

typedef struct Tokenizer {
    int num_tokens;
    int fd;
    Token tokens[MAX_TOKENS];
} Tokenizer;

typedef struct Values {
    int num_values;
    Token tokens[];
} Values;

Tokenizer load_tokens(const char* file_name);
Values* get_variable(Tokenizer t, const char* var);


#endif
