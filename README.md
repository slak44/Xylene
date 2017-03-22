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
- text handling
  - UTF-8 aware String class
  - byte arrays
  - maybe UnsafeString for user input?
- modules, import, export
- type system
  - unit type?
  - bottom type?
  - top type?
  - nullability
    - explicit option type?
    - null keyword?
    - allow variables to be null?
    - operating with null vars?
  - typedefs (can alias a typelist)
    - let the lists' types either add their contained types to the larger typelist, or only add the meta-type to the larger typelist
  - operating with variants
    - get the concrete type of a value as a string (function `typeOf(variable)` or unary operator `typeof variable`?)
    - concretize variant to type
    - get value as type by type name, throw error if the stored type isn't what the programmer expects (this should be a language construct)
    - even if functions specify a sum type as a return type, at runtime, the value returned has a non-composite type
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
  - disallow declarations starting with `_xyl_`?
  - multiple modules linker name conflicts
  - multiple scope name conflicts
  - llvm ir struct name conflicts
- try-catch, throw, exceptions (finally block?)
- first-class support for hashmaps
- lambdas and anon funcs
- operator overloading
- switch / pattern matcher
- standard library
- metaprogramming
  - templates?
  - macros?
- reflection...?
