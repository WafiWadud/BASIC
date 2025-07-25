# BASIC to C Translator Language Specification

## Overview
This specification defines a simplified BASIC language that translates to C code. The language supports variables, functions, control structures, and basic I/O operations.

## General Syntax Rules
- Commands are case-insensitive (`PRINT`, `print`, `Print` are all valid)
- Statements are processed line by line
- Whitespace is generally ignored except for separating tokens
- Comments are not currently supported

## Data Types
- **Integer**: All variables are treated as `int` in the generated C code
- **String Literals**: Enclosed in double quotes (`"Hello World"`)

## Variable Operations

### Variable Declaration
```basic
LET variable = value
```
- Declares and initializes an integer variable
- **Example**: `LET X = 5` → `int X = 5;`
- **Example**: `LET Y = 0` → `int Y = 0;`

### Variable Assignment
```basic
variable = value
```
- Assigns a value to an existing variable
- **Example**: `X = 10` → `X = 10;`
- **Example**: `RESULT = ADD(5, 3)` → `RESULT = ADD(5, 3);`

### Variable Modification
```basic
CHANGE variable = value
```
- Alternative syntax for variable assignment
- **Example**: `CHANGE X = 20` → `X = 20;`

## Input/Output Operations

### Print Statement
```basic
PRINT expression
```
- Outputs a value to the console
- For variables: `PRINT X` → `printf("%d\n", X);`
- For string literals: `PRINT "Hello"` → `printf("%s\n", "Hello");`

### Input Statement
```basic
INPUT variable
```
- Reads an integer from the user
- **Example**: `INPUT X` → `scanf("%d", &X);`

## Control Structures

### Conditional Statements
```basic
IF condition THEN statement
```
- Executes statement if condition is true
- **Example**: `IF X > 5 THEN PRINT "Large"` → 
  ```c
  if (X > 5) {
    printf("%s\n", "Large");
  }
  ```

### For Loops
```basic
FOR variable = start TO end [STEP step]
  statements
NEXT variable
```

**Components:**
- `variable`: Loop counter (automatically declared as int)
- `start`: Initial value
- `end`: Final value (inclusive)
- `step`: Optional increment/decrement (default: 1)

**Examples:**
```basic
FOR I = 1 TO 10
  PRINT I
NEXT I
```
→
```c
for (int I = 1; I <= 10; I++) {
  printf("%d\n", I);
}
```

```basic
FOR I = 10 TO 1 STEP -1
  PRINT I
NEXT I
```
→
```c
for (int I = 10; I >= 1; I--) {
  printf("%d\n", I);
}
```

```basic
FOR I = 0 TO 20 STEP 2
  PRINT I
NEXT I
```
→
```c
for (int I = 0; I <= 20; I += 2) {
  printf("%d\n", I);
}
```

## Functions

### Function Definition
```basic
FUNCTION name(param1, param2, ...)
  statements
  RETURN value
ENDFUNCTION
```

**Rules:**
- All parameters are integers
- Functions must return an integer value
- Function names are case-sensitive in the generated C code

**Example:**
```basic
FUNCTION ADD(A, B)
  RETURN A + B
ENDFUNCTION
```
→
```c
int ADD(int A, int B) {
  return A + B;
}
```

### Function Calls

#### Direct Call
```basic
CALL function(arguments)
```
- **Example**: `CALL PRINT_SUM(5, 3)` → `PRINT_SUM(5, 3);`

#### Assignment Call
```basic
variable = function(arguments)
```
- **Example**: `RESULT = ADD(5, 3)` → `RESULT = ADD(5, 3);`

### Return Statement
```basic
RETURN [expression]
```
- Returns a value from a function
- **Example**: `RETURN X * 2` → `return X * 2;`
- **Example**: `RETURN` → `return;`

## Program Structure

### Global Scope
- Function definitions are placed at the global scope
- All non-function statements are automatically placed in `main()`

### Generated C Structure
```c
#include <stdio.h>

// Function definitions appear here
int FUNCTION_NAME(parameters) {
  // function body
}

int main() {
  // All non-function statements appear here
  return 0;
}
```

## Complete Example

### BASIC Code:
```basic
FUNCTION FACTORIAL(N)
  LET RESULT = 1
  FOR I = 1 TO N
    RESULT = RESULT * I
  NEXT I
  RETURN RESULT
ENDFUNCTION

LET NUM = 5
LET FACT = FACTORIAL(NUM)
PRINT "Factorial of"
PRINT NUM
PRINT "is"
PRINT FACT
```

### Generated C Code:
```c
#include <stdio.h>

int FACTORIAL(int N) {
  int RESULT = 1;
  for (int I = 1; I <= N; I++) {
    RESULT = RESULT * I;
  }
  return RESULT;
}

int main() {
  int NUM = 5;
  int FACT = FACTORIAL(NUM);
  printf("%s\n", "Factorial of");
  printf("%d\n", NUM);
  printf("%s\n", "is");
  printf("%d\n", FACT);
  return 0;
}
```

## Operator Support

### Arithmetic Operators
- `+` (addition)
- `-` (subtraction)
- `*` (multiplication)
- `/` (division)

### Comparison Operators
- `>` (greater than)
- `<` (less than)
- `>=` (greater than or equal)
- `<=` (less than or equal)
- `==` (equal)
- `!=` (not equal)

### Logical Operators
- `&&` (and)
- `||` (or)
- `!` (not)

## Limitations

1. **No Arrays**: Only scalar integer variables are supported
2. **No Strings**: String literals can only be used in PRINT statements
3. **No ELSE**: Only simple IF-THEN statements are supported
4. **No WHILE/DO Loops**: Only FOR loops are implemented
5. **No Nested Functions**: Functions cannot be defined inside other functions
6. **No Recursion Optimization**: Recursive functions work but without optimization
7. **No Error Handling**: Invalid syntax may produce incorrect C code

## Error Messages

The translator provides basic error messages for common syntax errors:
- `// Syntax error: IF without THEN`
- `// Syntax error: FOR without TO`
- `// Syntax error: LET without variable`
- `// Command not recognized: [command]`

## Usage

1. Run the translator
2. Enter BASIC commands at the `BASIC>` prompt
3. Press Ctrl+C or Ctrl+D to finish
4. The generated C code is written to `output.c`
5. Compile with: `gcc output.c -o program`
6. Run with: `./program`