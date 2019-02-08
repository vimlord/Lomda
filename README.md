# Lomda
Lomda is a mathematically inspired programming language designed to be easily
readable and mathematically useful. The language is able to implicitly perform
a number of operations, ranging from addition to differential calculus. It also
comes with a type system that can be used to verify some of the safety of the
language.

A primarily motivation of the programming language was to enable easy
implementations of machine learning algorithms. In particular, the differential
operator enables simple gradient descent.

## Installation
As Lomda runs on a C++ backend, you will require g++11 to compile the source.
You will also require `cmake` to build the files. Installation can be
performed as follows:

```
# Create a build directory
mkdir build
cd build
# Generate a makefile
cmake ..
# Run the makefile to build Lomda.
make install
```

This will install the Lomda executable in /usr/local/bin/. To create the
executable in the current directory, run `make` instead of `make install`.
To test Lomda, you can run the interpreter with the test flag. That is,

```
$ lomda -t
```

## Usage
Lomda offers the ability to run either as a line-by-line interpreter or as an executor. The following are supported usages of the executable:

```
$ lomda
$ lomda source.lom
```

Without a source file, Lomda will initialize the interpreter and prompt for a
program or an exit command. The interpreter will look something like this:

```
$ lomda
Lomda v1.0.1
Compiled Feb  8 2019 @ 16:02:01
Enter a program and press <enter> to execute, or one of the following:
'exit' - exit the interpreter
'q/quit' - exit the interpreter
```

If you see the prompt, then you can begin typing in programs to execute. If you
are unfamiliar with the syntax, feel free to consult the [wiki](https://github.com/themaddoctor1/Lomda/wiki).

## Docs
For information regarding usage of the interpreter, please consult the [wiki](https://github.com/themaddoctor1/Lomda/wiki).
It features information on usage of the interpreter on the command line, as well as instructions regarding how to utilize
the syntax of the language.

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Credits
Lomda was created by Christopher Hittner

