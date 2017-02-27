# Xylene

A small toy programming language.

Documentation: https://slak44.github.io/Xylene/

## Dependencies

- cmake 3.6.1+
- llvm 3.9.1

Everything else is downloaded and installed before building by cmake.

## Language Syntax

EBNF grammar can be found [here](https://github.com/slak44/Xylene/blob/master/grammar.ebnf).

### TODO list
- strings
- disallow declarations starting with `_xyl_`
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
- for loop: multiple declarations & multiple update expressions
- properly define an identifier
- prefix notation in arguments
- allow default type for method definitions?
- name mangling
  - multiple modules linker name conflicts
  - multiple scope name conflicts
- try-catch, throw, exceptions, finally block?
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
