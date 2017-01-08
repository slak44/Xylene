# Xylene

A small toy programming language.
Documentation: https://slak44.github.io/Xylene/

## Language Syntax

EBNF-ish format of a program:
```
program = block ;
block = [ statement, ";", { statement, ";" } ] ;
statement =
  declaration | function | for_loop | while_loop | block | if_statement | try_catch | throw_statement |
  expression | "break" | "continue" | import_statement | export_statement | type_definition | native_fun_decl ;
declaration = "define" | type_list, ident, [ "=", expression ] ;
for_loop = "for", [ declaration, { ",", declaration } ], ";", [ expression ], ";", [ expression, { ",", expression } ], "do", block, "end" ;
while_loop = "while", expression, "do", block, "end" ;
if_statement = "if", expression, "do", block, [ "else", block | if_statement ] | "end" ;
type_definition = "type", ident, [ "inherits", [ "from" ], type_list ], "do", [ contructor_definition ], [ { method_definition | member_definition } ], "end" ;
constructor_definition = [ visibility_specifier ], [ "foreign" ], "constructor", [ "[", argument, { ",", argument }, "]" ], "do", block, "end" ;
method_definition = visibility_specifier, [ "static" ], [ "foreign" ], "method", function_signature, "do", block, "end" ;
member_definition = [ visibility_specifier ], [ "static" ], declaration ;
try_catch = "try", block, "catch", type_list, ident, "do", block, "end" ;
throw_statement = "throw", expression ;
import_statement = "import", ( ident, { ",", ident } ) | "all", [ "as", ident ], "from", ident ;
export_statement = "export", ident, "as", ident ;
typedef = "define", ident, "as", type_list ;
native_fun_decl = "foreign", "function", function_signature, ";" ;

function_signature = [ ident ], [ "[", argument, { ",", argument }, "]" ], [ "=>", type_list ] ;
function = [ "foreign" ], "function", function_signature, "do", block, "end" ;
visibility_specifier = "public" | "private" | "protected" ;
type_list = ident, {",", ident} ;
argument = ( type_list, ident ) ;
expression = primary, { binary_op, primary } ;
primary = { prefix_op }, terminal, { postfix_op | function_call } ;
function_call = "(", [ expression, { ",", expression } ], ")" ;
binary_op = ? see binary operators in operator.hpp ? ;
prefix_op = ? see prefix operators in operator.hpp ? ;
postfix_op = ? see postfix operators in operator.hpp ? ;
terminal = integer_literal | float_literal | boolean_literal | string_literal | identifier ;
integer_literal = ? decimal, binary, octal or hex integer, see Lexer class for details ? ;
float_literal = ? decimal floating point value, see Lexer class for details ?
boolean_literal = "true" | "false" ;
string_literal = ? quotation mark delimited string with backslash escape codes, see Lexer class for details ? ;
ident = ? see Lexer class for what is an identifier ? ;
```
TODO list:
- strings
- operating with variants
  - get the concrete type of a value as a string (function `typeOf(variable)` or unary operator `typeof variable`?)
  - concretize variant to type
  - get value as type by type name, throw error if the stored type isn't what the programmer expects (this should be a language construct)
- modules, import, export
- type system
  - type inference
  - inheritance
  - multiple inheritance?
  - interfaces?
  - abstract types?
  - enums (Java-like?)
  - for loop multiple declarations & multiple update expressions
- properly define an identifier
- name mangling
  - multiple modules linker name conflicts
  - multiple scope name conflicts
- try-catch, throw, exceptions
- typedefs
- first-class support for hashmaps
- lambdas and anon funcs
- operator overloading
- nullability
  - null keyword
  - allow variables to be null?
  - operating with null vars?
- switch / pattern matcher
- standard library
- metaprogramming
  - templates?
  - macros?
- reflection...?
