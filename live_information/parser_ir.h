#ifndef PARSER_IR_H
#define PARSER_IR_H

void lex_init(const char *text);
CFG parse_procedure(int *max_reg);

#endif
