#include "cargs.h"
#include <stdio.h>

static struct cag_option options[] = {
  {.identifier = 'u',
    .access_letters = "u",
    .access_name = "include-unsupported",
    .description = "Include file count for unsupported file types"},
  {.identifier = 'd',
    .access_letters = "d",
    .access_name = "include-dotfiles",
    .description = "Include hidden directories"},
  {.identifier = 'e',
    .access_letters = "e",
    .access_name = "exclude",
    .value_name = "<regex>",
    .description = "Skip paths matching given regex"},
  {.identifier = 'f',
    .access_letters = "f",
    .access_name = "filter",
    .value_name = "<regex>",
    .description = "Skip paths not matching given regex"},
  {.identifier = 'h',
    .access_letters = "h",
    .access_name = "help",
    .description = "Show this dialog"},
};

int main(int argc, char *argv[]) {
  cag_option_context ctx;
  cag_option_init(&ctx, options, CAG_ARRAY_SIZE(options), argc, argv);
  while (cag_option_fetch(&ctx)) {
    switch(cag_option_get_identifier(&ctx)) {
      case 's':
        printf("s printed!\n");
        return 0;
      case 'h':
        printf("Usage: lnstat <path> [options]...\n");
        printf("Utility for counting lines of source code, number of comment lines, and blank lines.\n\n");
        cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
        return 0;
    }
  }
}
