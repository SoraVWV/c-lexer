#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define is_number(c) (c >= '0' && c <= '9')
#define is_identifier_start(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$')
#define is_identifier(c) (is_identifier_start(c) || is_number(c))
#define is_number_bin(c) (c == '0' || c == '1')
#define is_number_hex(c) (is_number(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

TokenizerError error;

static TokenizerFrame collect_frame(const TokenizerContext *context) {
    const TokenizerFrame frame = {
        context->offset,
        context->line,
        context->column
    };

    return frame;
}

static char peek_n(const TokenizerContext *context, const int n) {
    const int offset = context->offset + n;

    if (offset >= context->content_length)
        return '\0';

    return context->content[offset];
}

static char peek(const TokenizerContext *context) {
    return peek_n(context, 0);
}

static char next(TokenizerContext *context) {
    const char result = peek(context);

    if (result == '\n') {
        context->line++;
        context->column = 1;
    } else {
        context->column++;
    }

    context->offset++;
    return result;
}

static char next_2(TokenizerContext *context) {
    next(context);
    return next(context);
}

static void skip_single_comment(TokenizerContext *context) {
    next_2(context);

    while (peek(context) != '\r' && peek(context) != '\n' && peek(context) != '\0')
        next(context);

    next(context);
}

static void skip_multi_comment(TokenizerContext *context) {
    const TokenizerFrame frame = collect_frame(context);
    next_2(context);

    while (peek_n(context, 0) != '*' || peek_n(context, 1) != '/') {
        if (context->offset >= context->content_length) {
            error.message = "unterminated comment";
            error.frame = frame;
            return;
        }

        next(context);
    }

    next_2(context);
}

static void skip(TokenizerContext *context) {
    for (;;) {
        if (peek_n(context, 0) == '/' && peek_n(context, 1) == '*')
            skip_multi_comment(context);
        else if (peek_n(context, 0) == '/' && peek_n(context, 1) == '/')
            skip_single_comment(context);
        else if (peek(context) == ' ' || peek(context) == '\t' || peek(context) == '\n' || peek(context) == '\r')
            next(context);
        else
            break;
    }
}

static char* slice(const TokenizerContext *context) {
    char *content = malloc(context->offset - context->frame.offset + 1);

    strncpy(content, &context->content[context->frame.offset], context->offset - context->frame.offset);
    content[context->offset - context->frame.offset] = '\0';
    return content;
}

static TokenType tokenize_identifier(TokenizerContext *context) {
    do {
        next(context);
    } while (is_identifier(peek(context)));

    const char *content = slice(context);

    if (strcmp(content, "is") == 0) return IS;
    if (strcmp(content, "as") == 0) return AS;
    if (strcmp(content, "package") == 0) return PACKAGE;
    if (strcmp(content, "import") == 0) return IMPORT;
    if (strcmp(content, "struct") == 0) return STRUCT;
    if (strcmp(content, "fn") == 0) return FN;
    if (strcmp(content, "return") == 0) return RETURN;
    if (strcmp(content, "var") == 0) return VAR;
    if (strcmp(content, "val") == 0) return VAL;
    if (strcmp(content, "if") == 0) return IF;
    if (strcmp(content, "else") == 0) return ELSE;
    if (strcmp(content, "for") == 0) return FOR;
    if (strcmp(content, "do") == 0) return DO;
    if (strcmp(content, "while") == 0) return WHILE;
    if (strcmp(content, "continue") == 0) return CONTINUE;
    if (strcmp(content, "break") == 0) return BREAK;
    if (strcmp(content, "this") == 0) return THIS;
    if (strcmp(content, "true") == 0) return TRUE;
    if (strcmp(content, "false") == 0) return FALSE;


    return IDENTIFIER;
}

static bool tokenize_dec_part(TokenizerContext *context, const bool req) {
    bool last_underscore = false;
    bool has_number = false;

    if (peek(context) == '_') {
        error.message = "numeric literal cannot have an underscore as it's first or last character";
        error.frame = context->frame;
        return false;
    }

    for (;;) {
        const char current_char = peek(context);

        if (is_number(current_char)) {
            has_number = true;
            last_underscore = false;
            next(context);
        } else if (current_char == '_') {
            last_underscore = true;
            next(context);
        } else {
            break;
        }
    }

    if (!has_number && req) {
        error.message = "numeric literal cannot terminate with a dot";
        error.frame = context->frame;
        return false;
    }

    if (last_underscore) {
        error.message = "numeric literal cannot have an underscore as it's last character";
        error.frame = context->frame;
        return false;
    }

    return true;
}

static TokenType tokenize_floating_point_number(TokenizerContext *context) {
    context->offset = context->frame.offset;
    context->line = context->frame.line;
    context->column = context->frame.column;

    if (!tokenize_dec_part(context, false)) {
        return ERROR;
    }

    if (peek(context) == '.') {
        next(context);
        if (!tokenize_dec_part(context, true)) {
            return ERROR;
        }
    }

    if (peek(context) == 'f' || peek(context) == 'F') {
        next(context);
        return FLOAT_NUMBER;
    }

    if (peek(context) == 'd' || peek(context) == 'D') {
        next(context);
    }

    return DOUBLE_NUMBER;
}

static TokenType tokenize_dec_number(TokenizerContext *context) {
    if (!tokenize_dec_part(context, true)) {
        return ERROR;
    }

    if (peek(context) == '.') {
        return tokenize_floating_point_number(context);
    }

    if (peek(context) == 'f' || peek(context) == 'F') {
        next(context);
        return FLOAT_NUMBER;
    }

    if (peek(context) == 'd' || peek(context) == 'D') {
        next(context);
        return DOUBLE_NUMBER;
    }

    if (peek(context) == 'l' || peek(context) == 'L') {
        next(context);
        return DEC_LONG_NUMBER;
    }

    return DEC_NUMBER;
}

// TODO bin, hex numbers
static TokenType tokenize_number(TokenizerContext *context) {
    if (peek_n(context, 0) == '.') {
        if (is_number(peek_n(context, 1))) {
            return tokenize_floating_point_number(context);
        }

        next(context);
        return DOT;
    }

    return tokenize_dec_number(context);
}

static bool tokenize_escape(TokenizerContext *context, const char allowed_char) {
    const TokenizerFrame frame = collect_frame(context);

    next(context);
    const char current = peek(context);
    switch (current) {
        case '\'':
        case '"':
            if (current != allowed_char) {
                break;
            }

        case 'n':
        case 't':
        case 'r':
        case '0':
        case '\\':
            next(context);
            return true;
        case 'u':
            next(context);
            for (int i = 0; i < 4; i++) {
                if (is_number_hex(peek(context))) {
                    next(context);
                    continue;
                }

                error.message = "invalid unicode";
                error.frame = frame;
                return false;
            }

            return true;
        default:
    }

    error.message = "invalid escape sequence";
    error.frame = frame;
    return false;
}

static TokenType tokenize_string(TokenizerContext *context) {
    next(context);

    while (peek(context) != '"') {
        if (context->offset >= context->content_length) {
            error.message = "string literal is not completed";
            error.frame = context->frame;
            return ERROR;
        }

        if (peek(context) == '\\') {
            tokenize_escape(context, '"');
        } else {
            next(context);
        }
    }
    next(context);

    return STRING_LITERAL;
}

static TokenType tokenize_char(TokenizerContext *context) {
    next(context);

    if (peek(context) == '\\') {
        if (!tokenize_escape(context, '\'')) {
            return ERROR;
        }
    } else {
        next(context);
    }

    if (next(context) != '\'') {
        error.message = "symbol literal is not completed";
        error.frame = context->frame;
        return ERROR;
    }

    return CHAR_LITERAL;
}

static TokenType tokenize_operator(TokenizerContext *context) {
    const char first = peek(context);
    next(context);

    switch (first) {
        case '(': return LEFT_PARENT;
        case ')': return RIGHT_PARENT;
        case '{': return LEFT_BRACE;
        case '}': return RIGHT_BRACE;
        case '[': return LEFT_SQUARE;
        case ']': return RIGHT_SQUARE;
        case ',': return COMMA;
        case ';': return SEMI;
        case ':': return COLON;
        case '?': return QUESTION_MARK;
        case '@': return AT_SYMBOL;
        case '~': return BIT_NOT;

        case '=': {
            if (peek(context) == '=') {
                next(context);
                return EQUALS;
            }
            return ASSIGN;
        }
        case '!': {
            if (peek(context) == '=') {
                next(context);
                return NOT_EQUALS;
            }
            return NOT;
        }
        case '>': {
            if (peek(context) == '=') {
                next(context);
                return GREATER_OR_EQUAL;
            }
            if (peek(context) == '>') {
                next(context);
                if (peek(context) == '=') {
                    next(context);
                    return BIT_SHIFT_RIGHT_ASSIGN;
                }
                return BIT_SHIFT_RIGHT;
            }
            return GREATER;
        }
        case '<': {
            if (peek(context) == '=') {
                next(context);
                return LESS_OR_EQUAL;
            }
            if (peek(context) == '<') {
                next(context);
                if (peek(context) == '=') {
                    next(context);
                    return BIT_SHIFT_LEFT_ASSIGN;
                }
                return BIT_SHIFT_LEFT;
            }
            return LESS;
        }
        case '&': {
            if (peek(context) == '&') {
                next(context);
                return AND;
            }
            if (peek(context) == '=') {
                next(context);
                return BIT_AND_ASSIGN;
            }
            return BIT_AND;
        }
        case '|': {
            if (peek(context) == '|') {
                next(context);
                return OR;
            }
            if (peek(context) == '=') {
                next(context);
                return BIT_OR_ASSIGN;
            }
            return BIT_OR;
        }
        case '^': {
            if (peek(context) == '=') {
                next(context);
                return BIT_XOR_ASSIGN;
            }
            return BIT_XOR;
        }
        case '+': {
            if (peek(context) == '+') {
                next(context);
                return INCREMENT;
            }
            if (peek(context) == '=') {
                next(context);
                return PLUS_ASSIGN;
            }
            return PLUS;
        }
        case '-': {
            if (peek(context) == '-') {
                next(context);
                return DECREMENT;
            }
            if (peek(context) == '=') {
                next(context);
                return MINUS_ASSIGN;
            }
            if (peek(context) == '>') {
                next(context);
                return IMPLICATION;
            }
            if ((PLUS <= context->type && context->type <= AS)
                || context->type == LEFT_PARENT
                || context->type == LEFT_SQUARE
                || context->type == RETURN
                || context->type == THIS) {
                return UNARY_MINUS;
            }

            return MINUS;
        }
        case '*': {
            if (peek(context) == '=') {
                next(context);
                return MULTIPLY_ASSIGN;
            }
            return MULTIPLY;
        }
        case '/': {
            if (peek(context) == '=') {
                next(context);
                return DIVIDE_ASSIGN;
            }
            return DIVIDE;
        }
        case '%': {
            if (peek(context) == '=') {
                next(context);
                return MODULO_ASSIGN;
            }
            return MODULO;
        }
        case '.': {
            if (is_number(peek(context))) {
                return tokenize_floating_point_number(context);
            }
            return DOT;
        }
        default:
    }

    error.message = "unknown operator";
    error.frame = context->frame;
    return ERROR;
}

Token *tokenizer_next(TokenizerContext *context) {
    skip(context);

    if (context->offset >= context->content_length || error.message) {
        return NULL;
    }

    context->frame = collect_frame(context);

    TokenType type;

    const char first = peek(context);
    if (is_identifier_start(first)) {
        type = tokenize_identifier(context);
    } else if (is_number(first) || first == '.') {
        type = tokenize_number(context);
    } else if (first == '"') {
        type = tokenize_string(context);
    } else if (first == '\'') {
        type = tokenize_char(context);
    } else {
        type = tokenize_operator(context);
    }

    if (type == ERROR) {
        return NULL;
    }

    context->type = type;

    Token *token = malloc(sizeof(Token));
    if (!token) {
        return NULL;
    }

    token->type = type;
    token->content = slice(context);
    token->offset = context->frame.offset;
    token->length = context->offset - context->frame.offset;
    token->line = context->frame.line;
    token->column = context->frame.column;

    return token;
}

TokenizerContext *tokenizer_init(const char *filename) {
    FILE *bin_file = fopen(filename, "rb");
    if (!bin_file) {
        return NULL;
    }

    fseek(bin_file, 0, SEEK_END);
    const long content_length = ftell(bin_file);
    fclose(bin_file);

    if (content_length < 0) {
        return NULL;
    }

    FILE *text_file = fopen(filename, "r");
    if (!text_file) {
        return NULL;
    }

    TokenizerContext *context = malloc(sizeof(TokenizerContext));
    if (!context) {
        fclose(text_file);
        return NULL;
    }

    char *content = malloc(content_length + 1);
    if (!content) {
        fclose(text_file);
        free(context);
        return NULL;
    }

    const int read_bytes = (int) fread(content, 1, content_length, text_file);
    if (ferror(text_file)) {
        free(content);
        free(context);
        fclose(text_file);
        return NULL;
    }

    content[read_bytes] = '\0';
    fclose(text_file);

    context->content = content;
    context->content_length = read_bytes;
    context->line = 1;
    context->column = 1;
    context->offset = 0;

    return context;
}

const char* token_type_to_string(const TokenType type) {
    switch (type) {
        case LEFT_PARENT: return "LEFT_PARENT";
        case RIGHT_PARENT: return "RIGHT_PARENT";
        case LEFT_BRACE: return "LEFT_BRACE";
        case RIGHT_BRACE: return "RIGHT_BRACE";
        case LEFT_SQUARE: return "LEFT_SQUARE";
        case RIGHT_SQUARE: return "RIGHT_SQUARE";
        case COMMA: return "COMMA";
        case DOT: return "DOT";
        case SEMI: return "SEMI";
        case COLON: return "COLON";
        case IMPLICATION: return "IMPLICATION";

        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MULTIPLY: return "MULTIPLY";
        case DIVIDE: return "DIVIDE";
        case MODULO: return "MODULO";

        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";

        case EQUALS: return "EQUALS";
        case NOT_EQUALS: return "NOT_EQUALS";
        case GREATER: return "GREATER";
        case LESS: return "LESS";
        case GREATER_OR_EQUAL: return "GREATER_OR_EQUAL";
        case LESS_OR_EQUAL: return "LESS_OR_EQUAL";

        case ASSIGN: return "ASSIGN";
        case PLUS_ASSIGN: return "PLUS_ASSIGN";
        case MINUS_ASSIGN: return "MINUS_ASSIGN";
        case MULTIPLY_ASSIGN: return "MULTIPLY_ASSIGN";
        case DIVIDE_ASSIGN: return "DIVIDE_ASSIGN";
        case MODULO_ASSIGN: return "MODULO_ASSIGN";

        case BIT_AND: return "BIT_AND";
        case BIT_OR: return "BIT_OR";
        case BIT_NOT: return "BIT_NOT";
        case BIT_XOR: return "BIT_XOR";
        case BIT_SHIFT_LEFT: return "BIT_SHIFT_LEFT";
        case BIT_SHIFT_RIGHT: return "BIT_SHIFT_RIGHT";

        case BIT_AND_ASSIGN: return "BIT_AND_ASSIGN";
        case BIT_OR_ASSIGN: return "BIT_OR_ASSIGN";
        case BIT_XOR_ASSIGN: return "BIT_XOR_ASSIGN";
        case BIT_SHIFT_LEFT_ASSIGN: return "BIT_SHIFT_LEFT_ASSIGN";
        case BIT_SHIFT_RIGHT_ASSIGN: return "BIT_SHIFT_RIGHT_ASSIGN";

        case UNARY_MINUS: return "UNARY_MINUS";
        case INCREMENT: return "INCREMENT";
        case DECREMENT: return "DECREMENT";

        case AT_SYMBOL: return "AT_SYMBOL";
        case QUESTION_MARK: return "QUESTION_MARK";
        case IS: return "IS";
        case AS: return "AS";

        case PACKAGE: return "PACKAGE";
        case IMPORT: return "IMPORT";
        case STRUCT: return "STRUCT";

        case FN: return "FN";
        case RETURN: return "RETURN";
        case VAL: return "VAL";
        case VAR: return "VAR";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case FOR: return "FOR";
        case DO: return "DO";
        case WHILE: return "WHILE";
        case CONTINUE: return "CONTINUE";
        case BREAK: return "BREAK";
        case THIS: return "THIS";

        case TRUE: return "TRUE";
        case FALSE: return "FALSE";

        case IDENTIFIER: return "IDENTIFIER";

        case HEX_LONG_NUMBER: return "HEX_LONG_NUMBER";
        case BIN_LONG_NUMBER: return "BIN_LONG_NUMBER";
        case DEC_LONG_NUMBER: return "DEC_LONG_NUMBER";
        case HEX_NUMBER: return "HEX_NUMBER";
        case BIN_NUMBER: return "BIN_NUMBER";
        case DEC_NUMBER: return "DEC_NUMBER";
        case FLOAT_NUMBER: return "FLOAT_NUMBER";
        case DOUBLE_NUMBER: return "DOUBLE_NUMBER";
        case CHAR_LITERAL: return "CHAR_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";

        case ERROR: return "ERROR";

        default: return "UNKNOWN_TOKEN";
    }
}

char* token_content_to_value(const Token *token) {
    const char *content = token->content;
    switch (token->type) {
        case IDENTIFIER:
            return strdup(content);

        case HEX_LONG_NUMBER:
        case BIN_LONG_NUMBER: {
            char *cleaned = strdup(content + 2);
            cleaned[strcspn(cleaned, "lL")] = '\0';
            return cleaned;
        }

        case DEC_LONG_NUMBER: {
            const size_t len = strlen(content) - 1;
            char *result = malloc(len + 1);
            memcpy(result, content, len);
            result[len] = '\0';
            return result;
        }

        case HEX_NUMBER:
        case BIN_NUMBER:
            return strdup(content + 2);

        case DEC_NUMBER:
            return strdup(content);

        case FLOAT_NUMBER:
        case DOUBLE_NUMBER: {
            char *cleaned = strdup(content);
            cleaned[strcspn(cleaned, "fFdD")] = '\0';
            return cleaned;
        }

        case CHAR_LITERAL:
        case STRING_LITERAL: {
            char *res = strdup(content + 1);
            res[strlen(res) - 1] = '\0';

            char *builder = malloc(strlen(res) * 4 + 1);
            int bi = 0;

            for (int i = 0; res[i]; ++i) {
                if (res[i] == '\\' && res[i + 1] == 'u') {
                    unsigned int code_point = 0;
                    sscanf(res + i + 2, "%4x", &code_point); // NOLINT(*-err34-c)
                    i += 5;

                    if (code_point >= 0xD800 && code_point <= 0xDBFF && res[i + 1] == '\\' && res[i + 2] == 'u') {
                        unsigned int low_surrogate = 0;
                        sscanf(res + i + 3, "%4x", &low_surrogate); // NOLINT(*-err34-c)
                        code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low_surrogate - 0xDC00);
                        i += 6;
                    }

                    if (code_point <= 0x7F) {
                        builder[bi++] = (char)code_point;
                    } else if (code_point <= 0x7FF) {
                        builder[bi++] = (char)(0xC0 | ((code_point >> 6) & 0x1F));
                        builder[bi++] = (char)(0x80 | (code_point & 0x3F));
                    } else if (code_point <= 0xFFFF) {
                        builder[bi++] = (char)(0xE0 | ((code_point >> 12) & 0x0F));
                        builder[bi++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
                        builder[bi++] = (char)(0x80 | (code_point & 0x3F));
                    } else {
                        builder[bi++] = (char)(0xF0 | ((code_point >> 18) & 0x07));
                        builder[bi++] = (char)(0x80 | ((code_point >> 12) & 0x3F));
                        builder[bi++] = (char)(0x80 | ((code_point >> 6) & 0x3F));
                        builder[bi++] = (char)(0x80 | (code_point & 0x3F));
                    }
                } else if (res[i] == '\\') {
                    switch (res[++i]) {
                        case 'n': builder[bi++] = '\n'; break;
                        case 'r': builder[bi++] = '\r'; break;
                        case 't': builder[bi++] = '\t'; break;
                        case 'b': builder[bi++] = '\b'; break;
                        case 'f': builder[bi++] = '\f'; break;
                        case '\\': builder[bi++] = '\\'; break;
                        case '\'': builder[bi++] = '\''; break;
                        case '"': builder[bi++] = '"'; break;
                        default: builder[bi++] = res[i]; break;
                    }
                } else {
                    builder[bi++] = res[i];
                }
            }

            builder[bi] = '\0';
            free(res);
            return builder;
        }

        default:
            return NULL;
    }
}

