#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void trim(char *str) {
  char *start = str;
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*start))
    start++;

  if (*start == 0) {
    // All spaces?
    *str = '\0';
    return;
  }

  // Move trimmed content to beginning of string
  if (start != str) {
    memmove(str, start, strlen(start) + 1);
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  // Write new null terminator
  *(end + 1) = '\0';
}

// Helper function to check if string starts with a word (case insensitive)
int strcasecmp_prefix(const char *str, const char *prefix) {
  while (*prefix && *str) {
    if (tolower(*str) != tolower(*prefix)) {
      return 0;
    }
    str++;
    prefix++;
  }
  return *prefix == '\0' && (isspace(*str) || *str == '\0');
}

char *interpret_basic(FILE *stream, char *input) {
  static char *static_input = NULL;
  static size_t len = 0;
  static char output[1024]; // Output buffer

  char *current_input;

  if (input == NULL) {
    ssize_t read = getline(&static_input, &len, stream);
    if (read == -1) {
      return NULL;
    }
    // Strip newline
    if (static_input[read - 1] == '\n')
      static_input[--read] = '\0';
    current_input = static_input;
  } else {
    current_input = input;
  }

  // Make a copy of the input for tokenization (strtok modifies the string)
  char *input_copy = strdup(current_input);
  if (!input_copy) {
    return NULL;
  }

  // Tokenize command
  char *cmd = strtok(input_copy, " ");
  char *args = strtok(NULL, ""); // Rest of the line

  if (!cmd) {
    free(input_copy);
    snprintf(output, sizeof(output), "// Empty command\n");
    return output;
  }

  // Check for variable assignment first (before LET)
  if (strchr(current_input, '=') && !strcasecmp_prefix(current_input, "LET") &&
      !strcasecmp_prefix(current_input, "FUNCTION") &&
      !strcasecmp_prefix(current_input, "FOR")) {
    // This is a direct assignment like: X = 5 or RESULT = ADD(5, 3)
    snprintf(output, sizeof(output), "%s;", current_input);
    free(input_copy);
    return output;
  }

  // let's interpret this
  if (strcasecmp(cmd, "PRINT") == 0) {
    if (args) {
      // Check if it's a string literal (starts and ends with quotes)
      if (args[0] == '"' && args[strlen(args) - 1] == '"') {
        snprintf(output, sizeof(output), "printf(\"%%s\\n\", %s);", args);
      } else {
        // It's a variable, use %d for integers
        snprintf(output, sizeof(output), "printf(\"%%d\\n\", %s);", args);
      }
    } else {
      snprintf(output, sizeof(output), "printf(\"\\n\");");
    }
  } else if (strcasecmp(cmd, "LET") == 0) {
    // Basic: LET X = 5  → C: int X = 5;
    if (args) {
      snprintf(output, sizeof(output), "int %s;", args);
    } else {
      snprintf(output, sizeof(output), "// Syntax error: LET without variable");
    }
  } else if (strcasecmp(cmd, "CHANGE") == 0) {
    // Basic: CHANGE X = 5  → C: X = 5;
    if (args) {
      snprintf(output, sizeof(output), "%s;", args);
    } else {
      snprintf(output, sizeof(output),
               "// Syntax error: CHANGE without assignment");
    }
  } else if (strcasecmp(cmd, "INPUT") == 0) {
    // Basic: INPUT X  → C: scanf("%d", &X);
    if (args) {
      snprintf(output, sizeof(output), "scanf(\"%%d\", &%s);", args);
    } else {
      snprintf(output, sizeof(output),
               "// Syntax error: INPUT without variable");
    }
  } else if (strcasecmp(cmd, "FUNCTION") == 0) {
    // Basic: FUNCTION ADD(A, B)  → C: int ADD(int A, int B) {
    if (args) {
      // Parse function name and parameters
      char *args_copy = strdup(args);
      char *paren_pos = strchr(args_copy, '(');
      if (paren_pos) {
        *paren_pos = '\0';
        char *func_name = args_copy;
        char *params = paren_pos + 1;

        // Remove closing parenthesis
        char *close_paren = strchr(params, ')');
        if (close_paren) {
          *close_paren = '\0';
        }

        trim(func_name);
        trim(params);

        // Convert parameters to C format
        if (strlen(params) > 0) {
          // Simple parameter conversion - assume all are integers
          char param_list[256] = "";
          char *param = strtok(params, ",");
          int first = 1;
          while (param) {
            trim(param);
            if (!first)
              strcat(param_list, ", ");
            strcat(param_list, "int ");
            strcat(param_list, param);
            first = 0;
            param = strtok(NULL, ",");
          }
          snprintf(output, sizeof(output), "int %s(%s) {", func_name,
                   param_list);
        } else {
          snprintf(output, sizeof(output), "int %s() {", func_name);
        }
      } else {
        // No parameters
        snprintf(output, sizeof(output), "int %s() {", args);
      }
      free(args_copy);
    } else {
      snprintf(output, sizeof(output),
               "// Syntax error: FUNCTION without name");
    }
  } else if (strcasecmp(cmd, "ENDFUNCTION") == 0) {
    // Basic: ENDFUNCTION  → C: } (mark as function end)
    snprintf(output, sizeof(output), "}ENDFUNCTION");
    free(input_copy);
    return output;
  } else if (strcasecmp(cmd, "RETURN") == 0) {
    // Basic: RETURN X  → C: return X;
    if (args) {
      snprintf(output, sizeof(output), "return %s;", args);
    } else {
      snprintf(output, sizeof(output), "return;");
    }
  } else if (strcasecmp(cmd, "FOR") == 0) {
    // Basic: FOR I = 1 TO 10 [STEP 2]  → C: for (int I = 1; I <= 10; I += 2) {
    if (args) {
      char *args_copy = strdup(args);
      char *variable = NULL;
      char *start_val = NULL;
      char *end_val = NULL;
      char *step_val = "1"; // Default step

      // Parse: variable = start TO end [STEP step]
      char *equals_pos = strchr(args_copy, '=');
      if (equals_pos) {
        *equals_pos = '\0';
        variable = args_copy;
        trim(variable);

        char *rest = equals_pos + 1;
        trim(rest);

        // Find TO keyword (case insensitive)
        char *to_pos = strcasestr(rest, "TO");
        if (to_pos) {
          *to_pos = '\0';
          start_val = rest;
          trim(start_val);

          char *end_part = to_pos + 2; // Skip "TO"
          trim(end_part);

          // Check for STEP keyword
          char *step_pos = strcasestr(end_part, "STEP");
          if (step_pos) {
            *step_pos = '\0';
            end_val = end_part;
            trim(end_val);

            step_val = step_pos + 4; // Skip "STEP"
            trim(step_val);
          } else {
            end_val = end_part;
            trim(end_val);
          }

          // Generate C for loop
          // Determine if step is positive or negative for correct comparison
          int step_num = atoi(step_val);
          char comparison[4];
          char increment[20];

          if (step_num >= 0) {
            strcpy(comparison, "<=");
            if (step_num == 1) {
              sprintf(increment, "%s++", variable);
            } else {
              sprintf(increment, "%s += %s", variable, step_val);
            }
          } else {
            strcpy(comparison, ">=");
            if (step_num == -1) {
              sprintf(increment, "%s--", variable);
            } else {
              sprintf(increment, "%s += %s", variable, step_val);
            }
          }

          snprintf(output, sizeof(output), "for (int %s = %s; %s %s %s; %s) {",
                   variable, start_val, variable, comparison, end_val,
                   increment);
        } else {
          snprintf(output, sizeof(output), "// Syntax error: FOR without TO");
        }
      } else {
        snprintf(output, sizeof(output),
                 "// Syntax error: FOR without assignment");
      }

      free(args_copy);
    } else {
      snprintf(output, sizeof(output),
               "// Syntax error: FOR without parameters");
    }
  } else if (strcasecmp(cmd, "NEXT") == 0) {
    // Basic: NEXT variable  → C: }
    // We ignore the variable name in C since the loop handles it
    snprintf(output, sizeof(output), "}");
  } else if (strcasecmp(cmd, "CALL") == 0) {
    // Basic: CALL MYFUNCTION(A, B)  → C: MYFUNCTION(A, B);
    if (args) {
      snprintf(output, sizeof(output), "%s;", args);
    } else {
      snprintf(output, sizeof(output),
               "// Syntax error: CALL without function");
    }
  } else if (strcasecmp(cmd, "IF") == 0) {
    if (!args) {
      free(input_copy);
      snprintf(output, sizeof(output), "// Syntax error: IF without condition");
      return output;
    }

    // Make a copy of args for manipulation
    char *args_copy = strdup(args);
    if (!args_copy) {
      free(input_copy);
      return NULL;
    }

    // Step 1: Find "THEN" in the args (case-insensitive)
    char *then_pos = strcasestr(args_copy, "THEN");
    if (!then_pos) {
      free(input_copy);
      free(args_copy);
      snprintf(output, sizeof(output), "// Syntax error: IF without THEN");
      return output;
    }

    // Step 2: Split the string
    *then_pos = '\0'; // terminate condition
    then_pos += 4;    // move past "THEN"

    // Skip any whitespace after THEN
    while (isspace((unsigned char)*then_pos))
      then_pos++;

    trim(args_copy); // args_copy now = condition

    // Step 3: Recursively interpret the THEN clause
    char *then_code = interpret_basic(NULL, then_pos);

    if (then_code) {
      // Remove any trailing newlines or semicolons from then_code
      char *clean_then = strdup(then_code);
      if (clean_then) {
        trim(clean_then);
        // Remove trailing semicolon if present
        int len = strlen(clean_then);
        if (len > 0 && clean_then[len - 1] == ';') {
          clean_then[len - 1] = '\0';
        }

        snprintf(output, sizeof(output), "if (%s) {\n    %s;\n  }", args_copy,
                 clean_then);
        free(clean_then);
      } else {
        snprintf(output, sizeof(output), "if (%s) {\n    %s\n  }", args_copy,
                 then_code);
      }
    } else {
      snprintf(output, sizeof(output), "// Error interpreting THEN clause");
    }

    free(args_copy);
  } else {
    snprintf(output, sizeof(output), "// Command not recognized: %s", cmd);
  }

  free(input_copy);
  return output;
}

int main(void) {
  FILE *out = fopen("output.c", "w");
  if (!out) {
    perror("fopen");
    return 1;
  }

  fprintf(out, "#include <stdio.h>\n\n");

  int in_main = 0;
  int in_function = 0;
  int function_count = 0;
  char **statements = NULL; // Buffer to store main statements
  int statement_count = 0;
  int statement_capacity = 100;

  // Allocate initial buffer for statements
  statements = malloc(statement_capacity * sizeof(char *));

  while (1) {
    printf("BASIC> ");
    fflush(stdout);

    char *c_line = interpret_basic(stdin, NULL);
    if (!c_line)
      break;

    printf("DEBUG: Processing line: '%s', in_function=%d\n", c_line,
           in_function);

    // Check if this is a function definition
    if (strstr(c_line, "int ") == c_line && strchr(c_line, '(') &&
        strchr(c_line, ')') && strchr(c_line, '{')) {
      // This is a function definition (starts with "int " and has parentheses
      // and opening brace)
      printf("DEBUG: Function definition detected\n");
      fprintf(out, "%s\n", c_line);
      in_function = 1;
      function_count++;
    } else if (strcmp(c_line, "}ENDFUNCTION") == 0 && in_function) {
      // End of function (ENDFUNCTION generates this)
      printf("DEBUG: Function end detected\n");
      fprintf(out, "}\n\n");
      in_function = 0;
    } else {
      // Regular statement
      if (in_function) {
        // We're inside a function definition
        printf("DEBUG: Statement in function: %s\n", c_line);
        fprintf(out, "  %s\n", c_line);
      } else {
        // We're in global scope - store for main function
        printf("DEBUG: Statement buffered for main: %s\n", c_line);
        if (statement_count >= statement_capacity) {
          statement_capacity *= 2;
          statements = realloc(statements, statement_capacity * sizeof(char *));
        }
        statements[statement_count] = strdup(c_line);
        statement_count++;
      }
    }
  }

  // Now write all main statements
  if (statement_count > 0) {
    fprintf(out, "int main() {\n");
    for (int i = 0; i < statement_count; i++) {
      fprintf(out, "  %s\n", statements[i]);
      free(statements[i]);
    }
    fprintf(out, "  return 0;\n}\n");
  } else if (function_count == 0) {
    // No functions defined and no statements, create empty main
    fprintf(out, "int main() {\n  return 0;\n}\n");
  }

  free(statements);
  fclose(out);
  printf("Translation complete. Output written to output.c\n");
  return 0;
}
