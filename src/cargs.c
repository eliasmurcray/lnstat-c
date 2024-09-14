#include <assert.h>
#include "cargs.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CAG_OPTION_PRINT_DISTANCE 4
#define CAG_OPTION_PRINT_MIN_INDENTION 20

static void cag_option_print_value(const cag_option *option,
  size_t *accessor_length, cag_printer printer, void *printer_ctx)
{
  if (option->value_name != NULL) {
    *accessor_length += printer(printer_ctx, "=%s", option->value_name);
  }
}

static void cag_option_print_letters(const cag_option *option, bool *first,
  size_t *accessor_length, cag_printer printer, void *printer_ctx)
{
  const char *access_letter;
  access_letter = option->access_letters;
  if (access_letter != NULL) {
    while (*access_letter) {
      if (*first) {
        *accessor_length += printer(printer_ctx, "-%c", *access_letter);
        *first = false;
      } else {
        *accessor_length += printer(printer_ctx, ", -%c", *access_letter);
      }
      ++access_letter;
    }
  }
}

static void cag_option_print_name(const cag_option *option, bool *first,
  size_t *accessor_length, cag_printer printer, void *printer_ctx)
{
  if (option->access_name != NULL) {
    if (*first) {
      *accessor_length += printer(printer_ctx, "--%s", option->access_name);
      *first = false;
    } else {
      *accessor_length += printer(printer_ctx, ", --%s", option->access_name);
    }
  }
}

static size_t cag_option_get_print_indention(const cag_option *options,
  size_t option_count)
{
  size_t option_index, indention, result;
  const cag_option *option;

  result = CAG_OPTION_PRINT_MIN_INDENTION;

  for (option_index = 0; option_index < option_count; ++option_index) {
    indention = CAG_OPTION_PRINT_DISTANCE;
    option = &options[option_index];
    if (option->access_letters != NULL && *option->access_letters) {
      indention += strlen(option->access_letters) * 4 - 2;
      if (option->access_name != NULL) {
        indention += strlen(option->access_name) + 4;
      }
    } else if (option->access_name != NULL) {
      indention += strlen(option->access_name) + 2;
    }

    if (option->value_name != NULL) {
      indention += strlen(option->value_name) + 1;
    }

    if (indention > result) {
      result = indention;
    }
  }

  return result;
}

void cag_option_init(cag_option_context *context, const cag_option *options,
  size_t option_count, int argc, char **argv)
{
  context->options = options;
  context->option_count = option_count;
  context->argc = argc;
  context->argv = argv;
  context->index = 1;
  context->inner_index = 0;
  context->forced_end = false;
  context->error_index = -1;
  context->error_letter = 0;
}

static const cag_option *cag_option_find_by_name(cag_option_context *context,
  char *name, size_t name_size)
{
  const cag_option *option;
  size_t i;

  for (i = 0; i < context->option_count; ++i) {
    option = &context->options[i];

    if (option->access_name == NULL) {
      continue;
    }

    if (strncmp(option->access_name, name, name_size) == 0
          && option->access_name[name_size] == '\0') {
      return option;
    }
  }

  return NULL;
}

static const cag_option *cag_option_find_by_letter(cag_option_context *context,
  char letter)
{
  const cag_option *option;
  size_t i;

  for (i = 0; i < context->option_count; ++i) {
    option = &context->options[i];

    if (option->access_letters == NULL) {
      continue;
    }

    if (strchr(option->access_letters, letter) != NULL) {
      return option;
    }
  }

  return NULL;
}

static void cag_option_parse_value(cag_option_context *context,
  const cag_option *option, char **c)
{
  if (option->value_name != NULL) {
    if (**c == '=') {
      context->value = ++(*c);
    } else {
      if (context->argc > context->index + 1) {
        ++context->index;
        *c = context->argv[context->index];
        context->value = *c;
      }
    }

    while (**c) {
      ++(*c);
    }
  }
}

static void cag_option_parse_access_name(cag_option_context *context, char **c)
{
  const cag_option *option;
  char *n;

  n = *c;
  while (**c && **c != '=') {
    ++*c;
  }

  assert(*c >= n);

  option = cag_option_find_by_name(context, n, (size_t)(*c - n));
  if (option != NULL) {
    context->identifier = option->identifier;

    cag_option_parse_value(context, option, c);
  } else {
    context->error_index = context->index;
  }

  ++context->index;
}

static void cag_option_parse_access_letter(cag_option_context *context,
  char **c)
{
  const cag_option *option;
  char *n, *v, letter;

  n = *c;

  letter = n[context->inner_index];
  option = cag_option_find_by_letter(context, letter);
  v = &n[++context->inner_index];
  if (option == NULL) {
    context->error_index = context->index;
    context->error_letter = letter;
  } else {
    context->identifier = option->identifier;

    cag_option_parse_value(context, option, &v);
  }

  if (*v == '\0') {
    ++context->index;
    context->inner_index = 0;
  }
}

static void cag_option_shift(cag_option_context *context, int start, int option,
  int end)
{
  char *tmp;
  int a_index, shift_index, left_shift, right_shift, target_index, source_index;

  left_shift = option - start;
  right_shift = end - option;

  if (left_shift == 0) {
    return;
  }

  for (a_index = option; a_index < end; ++a_index) {
    tmp = context->argv[a_index];

    for (shift_index = 0; shift_index < left_shift; ++shift_index) {
      target_index = a_index - shift_index;
      source_index = a_index - shift_index - 1;
      context->argv[target_index] = context->argv[source_index];
    }

    context->argv[a_index - left_shift] = tmp;
  }

  context->index = end - left_shift;

  if (context->error_index >= start) {
    if (context->error_index < option) {
      context->error_index += right_shift;
    } else if (context->error_index < end) {
      context->error_index -= left_shift;
    }
  }
}

static bool cag_option_is_argument_string(const char *c)
{
  return *c == '-' && *(c + 1) != '\0';
}

static int cag_option_find_next(cag_option_context *context)
{
  int next_index;
  char *c;

  next_index = context->index;

  if (next_index >= context->argc) {
    return -1;
  }

  c = context->argv[next_index];
  if (context->forced_end || c == NULL) {
    return -1;
  }

  while (!cag_option_is_argument_string(c)) {
    if (++next_index >= context->argc) {
      return -1;
    }

    c = context->argv[next_index];
    if (c == NULL) {
      return -1;
    }
  }

  return next_index;
}

bool cag_option_fetch(cag_option_context *context)
{
  char *c;
  int old_index, new_index;

  context->identifier = '?';
  context->value = NULL;
  context->error_index = -1;
  context->error_letter = 0;

  old_index = context->index;
  new_index = cag_option_find_next(context);
  if (new_index >= 0) {
    context->index = new_index;
  } else {
    return false;
  }

  c = context->argv[context->index];
  assert(*c == '-');
  ++c;

  if (*c == '-') {
    ++c;

    if (*c == '\0') {
      context->forced_end = true;
    } else {
      cag_option_parse_access_name(context, &c);
    }
  } else {
    cag_option_parse_access_letter(context, &c);
  }

  cag_option_shift(context, old_index, new_index, context->index);

  return context->forced_end == false;
}

char cag_option_get_identifier(const cag_option_context *context)
{
  return context->identifier;
}

const char *cag_option_get_value(const cag_option_context *context)
{
  return context->value;
}

int cag_option_get_index(const cag_option_context *context)
{
  return context->index;
}

CAG_PUBLIC int cag_option_get_error_index(const cag_option_context *context)
{
  return context->error_index;
}

CAG_PUBLIC char cag_option_get_error_letter(const cag_option_context *context)
{
  return context->error_letter;
}

CAG_PUBLIC void cag_option_printer_error(const cag_option_context *context,
  cag_printer printer, void *printer_ctx)
{
  int error_index;
  char error_letter;

  error_index = cag_option_get_error_index(context);
  if (error_index < 0) {
    return;
  }

  error_letter = cag_option_get_error_letter(context);
  if (error_letter) {
    printer(printer_ctx, "Unknown option '%c' in '%s'.\n", error_letter,
      context->argv[error_index]);
  } else {
    printer(printer_ctx, "Unknown option '%s'.\n", context->argv[error_index]);
  }
}

CAG_PUBLIC void cag_option_printer(const cag_option *options,
  size_t option_count, cag_printer printer, void *printer_ctx)
{
  size_t option_index, indention, i, accessor_length;
  const cag_option *option;
  bool first;

  indention = cag_option_get_print_indention(options, option_count);

  for (option_index = 0; option_index < option_count; ++option_index) {
    option = &options[option_index];
    accessor_length = 0;
    first = true;

    printer(printer_ctx, "  ");

    cag_option_print_letters(option, &first, &accessor_length, printer,
      printer_ctx);
    cag_option_print_name(option, &first, &accessor_length, printer,
      printer_ctx);
    cag_option_print_value(option, &accessor_length, printer, printer_ctx);

    for (i = accessor_length; i < indention; ++i) {
      printer(printer_ctx, " ");
    }

    printer(printer_ctx, " %s\n", option->description);
  }
}

#ifndef CAG_NO_FILE
CAG_PUBLIC void cag_option_print_error(const cag_option_context *context,
  FILE *destination)
{
  cag_option_printer_error(context, (cag_printer)fprintf, destination);
}
#endif

#ifndef CAG_NO_FILE
void cag_option_print(const cag_option *options, size_t option_count,
  FILE *destination)
{
  cag_option_printer(options, option_count, (cag_printer)fprintf, destination);
}
#endif

void cag_option_prepare(cag_option_context *context, const cag_option *options,
  size_t option_count, int argc, char **argv)
{
  cag_option_init(context, options, option_count, argc, argv);
}

char cag_option_get(const cag_option_context *context)
{
  return cag_option_get_identifier(context);
}
