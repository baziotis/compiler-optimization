#include <stdio.h>
#include "stefanos.h"
#include "bitset.h"
#include "cfg.h"
#include "parser_ir.h"
#include "liveout.h"

typedef struct EntireFile {
	char *contents;
	size_t contents_size;
} EntireFile;

EntireFile read_entire_file(const char *filename) {
	EntireFile file;
	FILE *handle = fopen(filename, "rb");
	assert(handle);
	
	ssize_t start = ftell(handle);
	fseek(handle, 0, SEEK_END);
	ssize_t end = ftell(handle);
	size_t file_size = end - start;
	
	fseek(handle, 0, SEEK_SET);
	file.contents = (char*) malloc(file_size + 1);
	assert(file.contents);
	file.contents_size = file_size;
	
	fread(file.contents, sizeof(char), file_size, handle);
  file.contents[file_size] = 0;
	
	fclose(handle);
	return file;
}

int main(int argc, char **argv) {
  assert(argc == 2);
  EntireFile file = read_entire_file(argv[1]);
  lex_init(file.contents);
  
  int max_register;
  CFG cfg = parse_procedure(&max_register);
  if (buf_len(cfg.bbs)) {
    BitSet *LiveOut = liveout_info(cfg, max_register);
    liveout_free(LiveOut);
  }

  cfg_destruct(&cfg);

  free(file.contents);
}
