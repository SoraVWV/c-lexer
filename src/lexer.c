#include <json_writer.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer/tokenizer.h"

extern TokenizerError error; // from "tokenizer/tokenizer.c"

typedef struct {
    char *input_file;
    char *output_file;
} LexerConfig;

static LexerConfig lexer_config_init(const int argc, char **argv) {
    LexerConfig config = {NULL, NULL};

    for (int i = 1; i < argc;) {
        if (strcmp(argv[i], "-i") == 0) {
            config.input_file = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], "-o") == 0) {
            config.output_file = argv[i + 1];
            i += 2;
        } else {
            i += 1;
        }
    }

    return config;
}

int main(const int argc, char **argv) {
    const LexerConfig config = lexer_config_init(argc, argv);

    if (!config.input_file) {
        fprintf(stderr, "Input file not specified\n");
        return 1;
    }

    if (!config.output_file) {
        fprintf(stderr, "Output file not specified\n");
        return 1;
    }

    TokenizerContext *context = tokenizer_init(config.input_file);
    if (!context) {
        fprintf(stderr, "Failed to read input file\n");
        return 1;
    }

    JsonWriter *jw = jw_open(config.output_file);
    if (!jw) {
        fprintf(stderr, "Failed to open output file\n");
        return 1;
    }

    jw_style_pretty_tabs(jw);
    jw_style_escape_unicode(jw, true);
    jw_object_start(jw);

    {
        jw_key(jw, "tokens");
        jw_array_start(jw);
        {
            Token *token = tokenizer_next(context);
            while (token) {
                jw_object_start(jw);
                {
                    char *content = token_content_to_value(token);
                    jw_key(jw, "type"); jw_string(jw, token_type_to_string(token->type));
                    switch (token->type) {
                        case STRING_LITERAL:
                        case CHAR_LITERAL:
                        case IDENTIFIER:
                            jw_key(jw, "content"); jw_string(jw, content);
                            break;
                        case DEC_NUMBER: {
                            char *ptr;
                            const int number = strtol(content, &ptr, 10);
                            if (*ptr != '\0' || ptr == content) {
                                fprintf(stderr, "Failed to parse decimal number '%s'\n", content);
                                return 1;
                            }

                            jw_key(jw, "content"); jw_integer(jw, number);
                            break;
                        }
                        case DEC_LONG_NUMBER: {
                            char *ptr;
                            const long long number = strtoll(content, &ptr, 10);
                            if (*ptr != '\0' || ptr == content) {
                                fprintf(stderr, "Failed to parse long decimal number '%s'\n", content);
                                return 1;
                            }

                            jw_key(jw, "content"); jw_long(jw, number);
                            break;
                        }
                        case FLOAT_NUMBER: {
                            char *ptr;
                            const float number = strtof(content, &ptr);
                            if (*ptr != '\0' || ptr == content) {
                                fprintf(stderr, "Failed to parse float number '%s'\n", content);
                                return 1;
                            }

                            jw_key(jw, "content"); jw_float(jw, number);
                            break;
                        }
                        case DOUBLE_NUMBER: {
                            char *ptr;
                            const double number = strtod(content, &ptr);
                            if (*ptr != '\0' || ptr == content) {
                                fprintf(stderr, "Failed to parse double number '%s'\n", content);
                                return 1;
                            }

                            jw_key(jw, "content"); jw_double(jw, number);
                            break;
                        }
                        default:
                    }
                    jw_key(jw, "offset"); jw_integer(jw, token->offset);
                    jw_key(jw, "length"); jw_integer(jw, token->length);
                    jw_key(jw, "line"); jw_integer(jw, token->line);
                    jw_key(jw, "column"); jw_integer(jw, token->column);
                    if (content) {
                        free(content);
                    }
                }
                jw_object_end(jw);

                free(token->content);
                free(token);

                token = tokenizer_next(context);
            }
        }
        jw_array_end(jw);

        jw_key(jw, "error");
        if (error.message) {
            jw_object_start(jw);
            {
                jw_key(jw, "message"); jw_string(jw, error.message);
                jw_key(jw, "offset"); jw_integer(jw, error.frame.offset);
                jw_key(jw, "line"); jw_integer(jw, error.frame.line);
                jw_key(jw, "column"); jw_integer(jw, error.frame.column);
            }
            jw_object_end(jw);
        } else {
            jw_null(jw);
        }
    }
    jw_object_end(jw);
    jw_close(jw);

    return 0;
}