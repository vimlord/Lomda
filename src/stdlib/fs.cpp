#include "stdlib.hpp"

#include <fcntl.h>
#include <unistd.h>

#include "expression.hpp"

using namespace std;

Val std_fclose(Env env) {
    Val f = env->apply("fd");
    
    int fd;
    if (!isVal<IntVal>(f)) {
        throw_err("type", "fs.close : S -> Z -> Z cannot be applied to argument " + f->toString());
        return NULL;
    } else
        fd = ((IntVal*) f)->get();

    throw_debug("fs", "closing file descriptor fd=" + to_string(fd));
    
    // Get a file descriptor
    int n = close(fd);

    return new IntVal(n);
}

Val std_fopen(Env env) {
    Val n = env->apply("path");
    Val f = env->apply("flags");

    std::string path = n->toString();
    int flags;
    if (!isVal<IntVal>(f)) {
        throw_err("type", "fs.open : S -> Z -> Z cannot be applied to argument " + f->toString());
        return NULL;
    } else
        flags = ((IntVal*) f)->get();
    
    // Get a file descriptor
    int fd = open(path.c_str(), flags | O_CREAT, 0666);

    throw_debug("fs", fd < 0
            ? "failed to open file at " + path
            : "opened file at " + path + " (fd=" + to_string(fd) + ")");

    return new IntVal(fd);
}

Val std_fread(Env env) {
    Val file = env->apply("fd");
    Val size = env->apply("n");

    if (!isVal<IntVal>(file)) {
        throw_err("type", "fs.read : Z -> Z -> S cannot be applied to argument " + file->toString());
        return NULL;
    } else if (!isVal<IntVal>(size)) { 
        throw_err("type", "fs.read : Z -> Z -> S cannot be applied to argument " + size->toString());
        return NULL;
    }

    // Get a file descriptor
    int fd = ((IntVal*) file)->get();
    int n = ((IntVal*) size)->get();

    char *buff = new char[n+1];
    n = read(fd, buff, n * sizeof(char));
    buff[n > 0 ? n : 0] = '\0';

    string str = buff;
    delete[] buff;

    return new StringVal(str);
}

Val std_fwrite(Env env) {
    Val file = env->apply("fd");
    Val buff = env->apply("buff");

    if (!isVal<IntVal>(file)) {
        throw_err("type", "fs.read : Z -> Z -> S cannot be applied to argument " + file->toString());
        return NULL;
    }
    
    int fd = ((IntVal*) file)->get();
    string str = buff->toString();

    int n = write(fd, str.c_str(), str.length());
    
    return new IntVal(n);
}

Type* type_stdlib_fs() {
    return new DictType {
        {
            "close",
            new LambdaType("fd",
                new IntType, new IntType)
        }, {
            "open",
            new LambdaType("path",
                new StringType, 
            new LambdaType("flags",
                new IntType, new IntType))
        }, {
            "read",
            new LambdaType("fd",
                new IntType,
            new LambdaType("n",
                new IntType,
                new StringType))
        }, {
            "write",
            new LambdaType("fd",
                new IntType,
            new LambdaType("buff",
                new StringType, new IntType))
        }
    };
}

Val load_stdlib_fs() {
    return new DictVal {
        {
            "close",
            new LambdaVal(new std::string[3]{"fd", ""},
                new ImplementExp(std_fclose, NULL))
        }, {
            "open",
            new LambdaVal(new std::string[3]{"path", "flags", ""},
                new ImplementExp(std_fopen, NULL))
        }, {
            "read",
            new LambdaVal(new std::string[3]{"fd", "n", ""},
                new ImplementExp(std_fread, NULL))
        }, {
            "write",
            new LambdaVal(new std::string[3]{"fd", "buff", ""},
                new ImplementExp(std_fwrite, NULL))
        }
        , { "RDONLY", new IntVal(O_RDONLY) }
        , { "WRONLY", new IntVal(O_RDONLY) }
        , { "RDWR", new IntVal(O_RDWR) }
        , { "APPEND", new IntVal(O_APPEND) }
    };
}

