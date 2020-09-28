#ifndef PARSE_IR
#define PARSE_IR

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "cfg.h"
#include "stefanos.h"

typedef struct Location {
  int ln;
} Location;

Location loc;

[[ noreturn ]] static
void fatal_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("Fatal Error: %d: ", loc.ln);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

static
void warning(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  printf("Warning: ");
  vprintf(fmt, args);
  printf("\n");
  va_end(args);
  exit(1);
}

static
void encountered_newline() { ++loc.ln; }

typedef enum TOK {
  TOK_EOI,
  TOK_BR,
  TOK_COMMA,
  TOK_COLON,
  TOK_INTLIT,
  TOK_LARROW,
  TOK_LBL,
  TOK_NL,
  TOK_PLUS,
  TOK_PRINT,
  TOK_REG,
} TOK;

typedef struct Token {
  TOK kind;
  int val;
} Token;

static
void token_kind_print(TOK kind) {
  printf("`");
  switch (kind) {
  case TOK_EOI:
    printf("End of Input");
    break;
  case TOK_BR:
    printf("BR");
    break;
  case TOK_COMMA:
    printf(",");
    break;
  case TOK_COLON:
    printf(":");
    break;
  case TOK_INTLIT:
    printf("Integer Literal");
    break;
  case TOK_LARROW:
    printf("<-");
    break;
  case TOK_LBL:
    printf("Label");
    break;
  case TOK_NL:
    printf("Newline");
    break;
  case TOK_PLUS:
    printf("+");
    break;
  case TOK_PRINT:
    printf("PRINT");
    break;
  case TOK_REG:
    printf("Register");
    break;
  }
  printf("`");
}

static
void token_print(Token tok) {
  switch (tok.kind) {
  case TOK_INTLIT:
    printf("`Lit: %d`", tok.val);
    break;
  case TOK_LBL:
    printf("`.%d`", tok.val);
    break;
  case TOK_REG:
    printf("`%%%d`", tok.val);
    break;
  default:
    token_kind_print(tok.kind);
  }
}

static Token token;

static const char *input;

static
int is_midchar(char c) { return isalpha(c) || c == '_'; }

static
int scan_int() {
  int base = 10;
  // Compute value.
  int val = 0;
  while (1) {
    if (!isdigit(*input))
      break;
    int digit = *input - '0';
    val = val * base + digit;
    if (val < 0) {
      warning("Overflow in line: %d", loc.ln);
      while (isdigit(*input)) {
        ++input;
      }
      return 0;
    }
    ++input;
  }
  return val;
}

static
void next_token() {
lex_again:
  if (!input[0]) {
    token.kind = TOK_EOI;
    return;
  }
  // skip whitespace
  while (isspace(*input)) {
    if (*input == '\n') {
      encountered_newline();
      ++input;
      token.kind = TOK_NL;
      return;
    }
    input++;
  }
  if (!input[0])
    goto lex_again;

  switch (*input) {
  case '.': {
    ++input;
    assert(isdigit(*input));
    token.kind = TOK_LBL;
    token.val = scan_int();
  } break;

  case '0' ... '9': {
    token.kind = TOK_INTLIT;
    token.val = scan_int();
  } break;

  case 'B': {
    if (input[1] == 'R' && !is_midchar(input[2])) {
      input += 2;
      token.kind = TOK_BR;
      return;
    }
    assert(0);
  } break;

  case 'P': {
    if (input[1] == 'R' && input[2] == 'I' && input[3] == 'N' &&
        input[4] == 'T' && !is_midchar(input[5])) {
      input += 5;
      token.kind = TOK_PRINT;
      return;
    }
    assert(0);
  } break;

  case '%': {
    assert(isdigit(input[1]));
    ++input;
    token.kind = TOK_REG;
    token.val = scan_int();
  } break;
    /*
    case 'l':
    {
        if (input[1] == 'a'
        &&  input[2] == 'b'
        &&  input[3] == 'e'
        &&  input[4] == 'l'
        && !is_midchar(input[5])) {
            input += 5;
            token.kind = TOK_LABEL;
            return;
        }
        assert(0);
    } break;
    */

  case '<': {
    assert(input[1] == '-');
    input += 2;
    token.kind = TOK_LARROW;
    return;
  } break;

  case '\0':
  case ',':
  case ':':
  case '+': {
    switch (*input) {
    case '\0':
      token.kind = TOK_EOI;
      break;
    case ',':
      token.kind = TOK_COMMA;
      break;
    case ':':
      token.kind = TOK_COLON;
      break;
    case '+':
      token.kind = TOK_PLUS;
      break;
    default:
      printf("%c\n", *input);
      assert(0);
    }
    ++input;
  } break;

  // Comment
  case ';': {
    ++input;
    while (*input != 0 && *input != '\n') {
      ++input;
    }
    goto lex_again;
  } break;

  default:
    fatal_error("Unrecognized character %c", *input);
  }
}

static
void lex_init(const char *text) {
  input = text;
  loc.ln = 1;
  next_token();
}

static
int is_token(TOK kind) { return (token.kind == kind); }

static
int match_token(TOK kind) {
  if (is_token(kind)) {
    next_token();
    return 1;
  }
  return 0;
}

/// If the current token is not of kind `kind`, generate an
/// error and return `false`. Otherwise, return `true`.
static
void expect_token(TOK kind) {
  if (!match_token(kind)) {
    printf("Fatal Error: %d: Expected ", loc.ln);
    token_kind_print(kind);
    printf(", found ");
    token_kind_print(token.kind);
    printf("\n");
    exit(1);
  }
}

static int curr_bb = 0;

static
Value parse_value(void) {
  Value v;
  switch (token.kind) {
  case TOK_REG: {
    v = val_reg(token.val);
  } break;
  case TOK_INTLIT: {
    v = val_imm(token.val);
  } break;
  default:
    fatal_error("Expected either integer literal or register for Value");
  }
  next_token();
  return v;
}

static
Operation parse_operation(void) {
  Value lhs = parse_value();
  switch (token.kind) {
  case TOK_PLUS: {
    next_token();
  } break;
  case TOK_NL: {
    // Ignore it, the Instruction parser will eat it but
    // return to not continue parsing any more Operation.
    return op_simple(lhs);
  } break;
  default:
    fatal_error("Expected either `+` or end of operation (i.e. newline)");
  }
  // We know we have an add otherwise we would have returned.
  return op_add(lhs, parse_value());
}

static
int starts_value(void) { return (is_token(TOK_REG) || is_token(TOK_INTLIT)); }

static int __max_reg_used;

static
Instruction *parse_instruction(int bb_num, CFG cfg) {
  Instruction *i;
  switch (token.kind) {
  case TOK_REG: {
    int reg = token.val;
    next_token();
    expect_token(TOK_LARROW);
    i = Instruction::def(reg, parse_operation());
    __max_reg_used = MAX(__max_reg_used, reg);
  } break;
  case TOK_PRINT: {
    next_token();
    Operation op = parse_operation();
    if (op.kind != OP_SIMPLE) {
      fatal_error("Only simple Operations for PRINT");
    }
    i = Instruction::print(op);
  } break;
  case TOK_BR: {
    next_token();
    int lbl = token.val;
    if (match_token(TOK_LBL)) {
      cfg.add_edge(bb_num, lbl);
      i = Instruction::br_uncond(lbl);
    } else {
      assert(starts_value());
      Value val = parse_value();
      expect_token(TOK_COMMA);
      int lbl1 = token.val;
      expect_token(TOK_LBL);
      expect_token(TOK_COMMA);
      int lbl2 = token.val;
      expect_token(TOK_LBL);
      i = Instruction::br_cond(val, lbl1, lbl2);
      // Add edges to the CFG
      cfg.add_edge(bb_num, lbl1);
      cfg.add_edge(bb_num, lbl2);
    }
  } break;
  default:
    // Should never actually get here becase parse_instruction()
    // is only called if the line actually starts Instruction.
    fatal_error("Expected either register or PRINT as start of instruction");
  }
  if (!match_token(TOK_NL)) {
    fatal_error("Expected newline at the end of Instruction");
  }
  return i;
}

static
int starts_instruction(void) {
  return (is_token(TOK_REG) || is_token(TOK_PRINT) || is_token(TOK_BR));
}

static
void parse_bb(CFG cfg) {
  int bb_num = token.val;
  expect_token(TOK_LBL);
  if (bb_num != curr_bb) {
    fatal_error("Expected basic block to be numbered: %d\n", loc.ln, curr_bb);
    return;
  }
  ++curr_bb;
  expect_token(TOK_COLON);
  expect_token(TOK_NL);
  while (starts_instruction()) {
    cfg.bbs[bb_num].insts.insert_at_end(parse_instruction(bb_num, cfg));
  }
}

static
int count_bbs() {
  int res = 0;
  const char *runner = input;
  while (*runner) {
    if (*runner++ == ':')
      res++;
  }
  return res;
}

typedef struct EntireFile {
  char *contents;
  size_t contents_size;
} EntireFile;

static
EntireFile read_entire_file(const char *filename) {
  EntireFile file;
  FILE *handle = fopen(filename, "rb");
  assert(handle);

  ssize_t start = ftell(handle);
  fseek(handle, 0, SEEK_END);
  ssize_t end = ftell(handle);
  size_t file_size = end - start;

  fseek(handle, 0, SEEK_SET);
  file.contents = (char *)malloc(file_size + 1);
  assert(file.contents);
  file.contents_size = file_size;

  fread(file.contents, sizeof(char), file_size, handle);
  file.contents[file_size] = 0;

  fclose(handle);
  return file;
}

static
CFG parse_procedure(const char *filename, int *max_reg) {
  EntireFile file = read_entire_file(filename);
  lex_init(file.contents);

  int nbbs = count_bbs();
  CFG cfg(nbbs);
  printf("Number of BBs: %d\n", nbbs);
  if (nbbs) {
    int i = 0;
    while (is_token(TOK_LBL) || is_token(TOK_NL)) {
      while (is_token(TOK_NL))
        next_token();
      parse_bb(cfg);
      ++i;
    }
  }
  if (max_reg != NULL) {
    *max_reg = __max_reg_used;
  }

  free(file.contents);

  return cfg;
}

#endif
