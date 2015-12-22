# test-lang

### Dependencies

- `make`
- `gcc`
  - Note, MinGW for Windows cannot currently be used, because it does not support `std::to_string` and other string operations
- boost libs
  - Specifically, `boost/any.hpp`, `boost/program_options.hpp` and the `libboost_program_options` shared library
- `zip` (for bundling the windows release)

### Build Instructions

Just run `make all` to build with the default debug flags. Use `CONFIG=RELEASE` to enable optimizations.  
Other options:
- `EXECUTABLE`: name of linked executable
- `CC`: compiler command
- `DEBUG_FLAGS` and `RELEASE_FLAGS`: compiler flags
- `LDFLAGS`: linker flags

Running `make releases` creates a 64bit Linux build and a 32bit Windows one.  
The Linux executable is not statically linked with anything.  
The Windows executable only requires the bundled `libwinpthread` and `libboost_program_options` to be dynamically linked.

### Tests

Tests require `node` and `npm`.  
Run `npm install` in the tests directory to install dependencies.  
To execute tests, use `node cli.js --color`.

### Syntax And Behavior
- NOTE: not everything is implemented yet

#### Reserved Words And Characters

- TODO

#### Line Endings

Semicolons are optional.

#### Comments

Commenting the rest of the line: `// Comment`  
Multi-line comment:
```
/* comment
also comment */
```

#### Operators

A list of operators, complete with precedence, associativity and arity can be found in `operators.cpp`.

#### Operator Overloading

- TODO

#### Scope

Scope is lexical (hopefully).  
Blocks begin with `do` and end with `end`.
```
define i = 1;
if true do
  define k = 2;
end
i = k + 1; // Error
```

#### Literals

Integers: `420`, `-67`  
Floating point: `123.456`, `3.000000001`  
Strings: `"qwertyuiop[];1234"`  
Booleans: `true` or `false`  
Arrays (dynamic only): `[123, 6.7, "abcd"]`

#### Functions

Definition example:
```
define function substract [Integer, Float a] [Integer, Float b] => Integer, Float do
  return a - b;
end
```
Arguments can have zero, one or more types.  
Arguments must be enclosed in `[]`.
The return type can be specified with `=>`, but it is not mandatory.  
The type list can be specified either before or after the parameter name.

Example syntax for two parameters, one postfixed, one prefixed:
```
define function function_name [type_list1 parameter_name1] [parameter_name2: type_list2] => return_type_list do
  code
end
```

#### Conditional Structure

The keywords used are `if` and `else`.  
Syntax:
```
if expression do
  code
else
  code
end
```
Enclosing the `expression` in parenthesis is not mandatory, but the block begin/end must be specified even for a one line statement:
```
if 1 + 1 == 2 do print("2") end
```

#### Looping Structures

- While Loop
Syntax:
```
while expression do
  code
end
```
This loop behaves just like the conditional, except the code is executed in a loop until the `expression` evaluates to false.
- For Loop
Syntax:
```
for declarations; expression1; expression2 do
  code
end
```
The `declarations` add to the loop's scope.  
The loop executes until `expression1` is false.  
`expression2` is evaluated every iteration.

#### Types

Definition example:
```
define type AnotherInt do
  public static Float whatIsThisDoingHere = 6.5;
  private Integer internal;
  define constructor [Integer, Float int] do
    if a.type == Float do
      a = Integer(a);
    end
    internal = a;
  end
  define public function square => Integer do
    return internal = internal * internal;
  end
end
```
Visibility can be specified by using the keywords `private`, `protected` and `public`.  
A type definition must specify at least one constructor function, that has no return type.  
The type list and the specifiers can be in any order:
```
public static Float whatIsThisDoingHere = 6.5;
static Float public whatIsThisDoingHere = 6.5;
Float static public whatIsThisDoingHere = 6.5;
...
```
However, in methods, the `define` keyword must be the first one.

Type checking can be done by specifying one or more types in a declaration:  
```
Integer, Float number;
number = 42; // Ok
number = 1.5; // Also ok
number = "abcd123"; // TypeError
```
It can be ignored by using the `define` keyword:  
```
define thing = 123;
thing = "asdf";
thing = false;
```

#### Built-in Types And Functions

- TODO