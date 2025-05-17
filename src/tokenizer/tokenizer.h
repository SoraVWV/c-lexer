#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef enum {
    LEFT_PARENT,
    RIGHT_PARENT,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_SQUARE,
    RIGHT_SQUARE,
    COMMA,
    DOT,
    SEMI,
    COLON,
    IMPLICATION,

    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,

    AND,
    OR,
    NOT,

    EQUALS,
    NOT_EQUALS,
    GREATER,
    LESS,
    GREATER_OR_EQUAL,
    LESS_OR_EQUAL,

    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MULTIPLY_ASSIGN,
    DIVIDE_ASSIGN,
    MODULO_ASSIGN,

    BIT_AND,
    BIT_OR,
    BIT_NOT,
    BIT_XOR,
    BIT_SHIFT_LEFT,
    BIT_SHIFT_RIGHT,

    BIT_AND_ASSIGN,
    BIT_OR_ASSIGN,
    BIT_XOR_ASSIGN,
    BIT_SHIFT_LEFT_ASSIGN,
    BIT_SHIFT_RIGHT_ASSIGN,

    UNARY_MINUS,
    INCREMENT,
    DECREMENT,

    AT_SYMBOL,
    QUESTION_MARK,
    IS, // `obj is String`
    AS, // `obj as String`

    PACKAGE,
    IMPORT,
    STRUCT,

    FN,
    RETURN,
    VAL,
    VAR,
    IF,
    ELSE,
    FOR,
    DO,
    WHILE,
    CONTINUE,
    BREAK,
    THIS,

    TRUE,
    FALSE,

    IDENTIFIER,

    HEX_LONG_NUMBER,
    BIN_LONG_NUMBER,
    DEC_LONG_NUMBER,
    HEX_NUMBER,
    BIN_NUMBER,
    DEC_NUMBER,
    FLOAT_NUMBER,
    DOUBLE_NUMBER,
    CHAR_LITERAL,
    STRING_LITERAL,

    ERROR
} TokenType;

const char* token_type_to_string(TokenType type);

typedef struct {
    const char *keyword;
    TokenType token;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"package", PACKAGE},
    {"import", IMPORT},
    {"struct", STRUCT},
    {"fn", FN},
    {"return", RETURN},
    {"var", VAR},
    {"val", VAL},
    {"if", IF},
    {"else", ELSE},
    {"for", FOR},
    {"do", DO},
    {"while", WHILE},
    {"continue", CONTINUE},
    {"break", BREAK},
    {"this", THIS},
    {"true", TRUE},
    {"false", FALSE},
    {"and", AND},
    {"or", OR},
    {"not", NOT},
    {"is", IS},
    {"as", AS},
};

typedef struct {
    TokenType type;
    char *content;
    int offset;
    int length;
    int line;
    int column;
} Token;

char* token_content_to_value(const Token *token);

typedef struct {
    int offset;
    int line;
    int column;
} TokenizerFrame;

typedef struct {
    char *content;
    int content_length;
    TokenizerFrame frame;
    TokenType type;
    int offset;
    int line;
    int column;
} TokenizerContext;

typedef struct {
    char *message;
    TokenizerFrame frame;
} TokenizerError;

TokenizerContext *tokenizer_init(const char *filename);

Token *tokenizer_next(TokenizerContext *context);

#endif //TOKENIZER_H
