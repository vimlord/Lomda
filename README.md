# Lomda
Lomda is a mathematically inspired programming language designed to be easily readable and mathematically useful.
The language is able to implicitly perform a number of operations, ranging from addition to differential calculus.
It also comes with a type system that can be used to verify some of the safety of the language.

## Installation
On a Unix-based system, the source can be built using `make`. Simply run it in the main directory to generate the `lomda` executable.

## Usage
Lomda offers the ability to run either as a line-by-line interpreter or as an executor. The following are supported usages of the executable:

```
$ ./lomda
$ ./lomda source.lom
```

Without a source file, Lomda will initialize the interpreter and prompt for a
program or an exit command. The interpreter will look something like this:

```
$ ./lomda
Lomda 0.1.0
Compiled May  9 2018 @ 20:11:11
Enter a program and press <enter> to execute, or one of the following:
'exit' - exit the interpreter
'q/quit' - exit the interpreter
>
```

For more information, please consult the [wiki](https://github.com/themaddoctor1/Lomda/wiki).

## Docs
For information regarding usage of the interpreter, please consult the [wiki](https://github.com/themaddoctor1/Lomda/wiki).
It features information on usage of the interpreter on the command line, as well as instructions regarding how to utilize
the syntax of the language.

## Credits
Lomda was created by Christopher Hittner
