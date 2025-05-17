# Axolotl Lexer Library/CLI

A lexical analyzer for the Axolotl programming language, implemented in C. This library provides tokenization of Axolotl source code with precise location tracking.

## Features

* Complete coverage of Axolotl language syntax:
    * All punctuation and delimiters
    * Arithmetic, logical, and bitwise operators
    * Compound assignment operators
    * Unary operators (including special `is` and `as` operators)
    * Language keywords and identifiers
    * Numeric literals (decimal, hex, binary, floating-point)
    * Character and string literals
* Precise source location tracking:
    * Byte offset
    * Line and column numbers
* Error token generation for invalid inputs
* Simple iterator-style API

## API Usage

### Initialization

```c
#include "tokenizer.h"

// Initialize lexer with source file
TokenizerContext *ctx = tokenizer_init("source.axl");

// Check for initialization errors
if (ctx == NULL) {
    // handle error
}
```

### Token Processing

```c
Token *token;
while ((token = tokenizer_next(ctx)) != NULL) {
    // Process token
    printf("%s:%d:%d %s - %s\n",
        "source.axl",
        token->line,
        token->column,
        token_type_to_string(token->type),
        token->content);

    // Clean up token memory
    free(token->content);
    free(token);
}

// Clean up lexer
tokenizer_free(ctx);
```

### Token Information

The `Token` structure contains:

```c
typedef struct {
    TokenType type;     // Type from the enum
    char *content;      // Raw source content
    int offset;         // Byte offset in file
    int length;         // Length in characters
    int line;           // Line number (1-based)
    int column;         // Column number (1-based)
} Token;
```

## Axolotl Language Features Supported

### Operators

* Arithmetic: `+ - * / %`
* Comparison: `== != > < >= <=`
* Logical: `and or not && ||`
* Bitwise: `& | ^ ~ << >>`
* Assignment: `= += -= *= /= %= &= |= ^= <<= >>=`
* Special: `is as` (type operators), `++ --` (increment/decrement)

### Literals

* Numbers:
    * Integer: `42`, `0xFF`, `0b1010`
    * Floating-point: `3.14`, `6.022f`
* Characters: `'a'`, `'\n'`, `'\u0000'`
* Strings: `"hello"`, `"multi\nline"`
* Booleans: `true`, `false`

### Keywords

```
package import struct
fn return val var
if else for do while
continue break this
```

## Example: Tokenizing Axolotl Code

Input (`test.axl`):
```axolotl
package axl.example

struct Point {
    x: float
    y: float
}

fn distance(p: Point) -> float {
    return sqrt(p.x * p.x + p.y * p.y)
}
```

Tokenization output:
```
PACKAGE       "package"       (line 1)
IDENTIFIER    "axl"           (line 1)
DOT           "."             (line 1)
IDENTIFIER    "example"       (line 1)
STRUCT        "struct"        (line 3)
IDENTIFIER    "Point"         (line 3)
LEFT_BRACE    "{"             (line 3)
IDENTIFIER    "x"             (line 4)
COLON         ":"             (line 4)
FLOAT         "float"         (line 4)
IDENTIFIER    "y"             (line 5)
...
```

## Building

- Place tokenizer.h on your include path.
- Link liblexer.a when compiling.
- Include and invoke.
