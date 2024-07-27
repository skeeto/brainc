#include "compiler.h"

void free_compiler_memory(Instructions *ins)
{
  da_heap_free(ins);
  ins = NULL;
}

bool generate_assembly(Instructions *ins, const char *filename)
{
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    fprintf(stderr, "COMPILATION ERROR: couldn't generate assembly with %s filename.\n", filename);
    return false;
  }
  fprintf(f, "        .bss\n");
  fprintf(f, "bytesarr: .zero %d\n", MAX_BYTES);
  fprintf(f, "pos:    .long 0\n");

  fprintf(f, "        .text\n");
  fprintf(f, "        .globl _start\n");

  fprintf(f, "_start:\n");
  int ip = 0;
  bool res = false;
  Instruction in = ins->items[ip];
  while (in.kind != END_OF_FILE) {
    switch (in.kind) {
      case INCREMENT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  addb $%d, bytesarr(%%rbx)\n", in.value);
        in = ins->items[++ip];
      } break;

      case DECREMENT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  subb $%d, bytesarr(%%rbx)\n", in.value);
        in = ins->items[++ip];
      } break;

      case INPUT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        for (int i = 0; i < in.value; i++) {
          fprintf(f, "  xor %%eax, %%eax\n");
          fprintf(f, "  xor %%edi, %%edi\n");
          fprintf(f, "  lea rsi, bytesarr(%%rbx), %%rsi\n");
          fprintf(f, "  mov $1, %%rdx\n");
          fprintf(f, "  syscall\n");
        }
        in = ins->items[++ip];
      } break;

      case OUTPUT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        for (int i = 0; i < in.value; i++) {
          fprintf(f, "  mov $1, %%rax\n");
          fprintf(f, "  mov $1, %%rdi\n");
          fprintf(f, "  lea bytesarr(%%rbx), %%rsi\n");
          fprintf(f, "  mov $1, %%rdx\n");
          fprintf(f, "  syscall\n");
        }
        in = ins->items[++ip];
      } break;

      case SHIFT_RIGHT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  add $%d, %%ebx\n", in.value);
        fprintf(f, "  mov %%ebx, (pos)\n");
        in = ins->items[++ip];
      } break;

      case SHIFT_LEFT: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  sub $%d, %%ebx\n", in.value);
        fprintf(f, "  mov %%ebx, (pos)\n");
        in = ins->items[++ip];
      } break;

      case IF_ZERO: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  mov bytesarr(%%rbx), %%al\n");
        fprintf(f, "  test %%al, %%al\n");
        fprintf(f, "  jz addr_%d\n", in.value);
        fprintf(f, "addr_%d:\n", ip + 1);
        in = ins->items[++ip];
      } break;

     case IF_NZERO: {
        fprintf(f, "  mov (pos), %%ebx\n");
        fprintf(f, "  mov bytesarr(%%rbx), %%al\n");
        fprintf(f, "  test %%al, %%al\n");
        fprintf(f, "  jnz addr_%d\n", in.value);
        fprintf(f, "addr_%d:\n", ip + 1);
        in = ins->items[++ip];
      } break;

      default: {
        fprintf(stderr, "Reached the unreachable end of compilation.\n");
        res = false;
      } break;
    }
  }
  fprintf(f, "  mov $60, %%eax\n");
  fprintf(f, "  xor %%edi, %%edi\n");
  fprintf(f, "  syscall\n");
  res = true;
  fclose(f);
  free_compiler_memory(ins);
  return res;
}

bool compile_assembly(const char *output_name)
{
  if (system("as -g source.asm -o source.o") != 0) {
    fprintf(stderr, "COMPILATION ERROR: couldn't produce the object file.\n");
    return false;
  }
  if (strlen(output_name) > 1000) {
    fprintf(stderr, "COMPILATION ERROR: output filename length cannot be bigger than 1000.\n");
    return false;
  }
  char ld[MAX_OUTPUT_FILENAME];
  sprintf(ld, "ld source.o -O3 -o %s", output_name);
  if (system(ld) != 0) {
    fprintf(stderr, "COMPILATION ERROR: couldn't link the object file.\n");
    return false;
  }
  return true;
}

int compile(Instructions *ins, const char *output_name)
{
  if (!generate_assembly(ins, "source.asm")) return FAIL;
  if (!compile_assembly(output_name)) return FAIL;
  return SUCCESS;
}
