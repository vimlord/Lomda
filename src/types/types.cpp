#include "types.hpp"
#include "expression.hpp"
#include "proof.hpp"

#include "parser.hpp"

#include <fstream>

#include "stdlib.hpp"

using namespace std;

AlgebraicDataType::~AlgebraicDataType() {
    delete[] kinds;
    
    if (argss) {
        for (int i = 0; argss[i]; i++) {
            for (int j = 0; argss[i][j]; j++)
                delete argss[i][j];
            delete[] argss[i];
        }
        delete[] argss;
    }
}
bool AlgebraicDataType::isConstant(Tenv) { return true; }
Type* AlgebraicDataType::clone() {
    string *xs;
    Type ***ass;
    
    if (argss) {
        // Argument count
        int i;
        for (i = 0; argss[i]; i++);
        
        xs = new string[i+1];
        ass = new Type**[i+1];

        xs[i] = ""; ass[i] = NULL;
        
        // Generate duplicates of each of the subtypes.
        while (i--) {
            xs[i] = kinds[i];

            int j; for (j = 0; argss[i][j]; j++);
            ass[i] = new Type*[j+1];
            ass[i][j] = NULL;
            while (j--) ass[i][j] = argss[i][j]->clone();
        }
    } else {
        xs = NULL;
        ass = NULL;
    }
    
    return new AlgebraicDataType(name, xs, ass); 
}
// The type is constant, therefore cannot be reduced any further.
bool AlgebraicDataType::depends_on_tvar(string, Tenv) { return false; }
Type* AlgebraicDataType::simplify(Tenv tenv) {
    if (argss) {
        for (int i = 0; argss[i]; i++)
        for (int j = 0; argss[i][j]; j++) {
            auto T = argss[i][j]->simplify(tenv);
            if (!T) return NULL;
            else {
                delete argss[i][j];
                argss[i][j] = T;
            }
        }
    }
    return clone();
}

DictType::DictType(initializer_list<pair<string, Type*>> ts) {
    types = new HashMap<std::string, Type*>;

    // Add each of the items
    for (auto it : ts)
        types->add(it.first, it.second);
}

Type* DictType::clone() {
    auto ts = new HashMap<std::string, Type*>;

    auto tt = types->iterator();
    while (tt->hasNext()) {
        string k = tt->next();
        ts->add(k, types->get(k)->clone());
    }
    delete tt;
    
    return new DictType(ts);
}
Type* DictType::simplify(Tenv tenv) {
    auto ts = new HashMap<std::string, Type*>;
    auto T = new DictType(ts);

    auto tt = types->iterator();
    while (tt->hasNext()) {
        // Compute the subtype
        string k = tt->next();
        auto t = types->get(k)->simplify(tenv);
        if (t)
            // Substitute if possible
            ts->add(k, t);
        else {
            // The type is broken, thus illegal.
            delete T;
            T = NULL;
            break;
        }
    }
    delete tt;
    
    return T;
}

bool DictType::depends_on_tvar(string x, Tenv tenv) {
    auto it = types->iterator();
    
    while (it->hasNext()) {
        // Check each of the types involved to authenticate
        string k = it->next();
        auto t = types->get(k);
        if (t->depends_on_tvar(x, tenv)) {
            // One of the subtypes has a dependency
            delete it;
            return true;
        }
    }
    
    // No type matches
    delete it;
    return false;
}

bool VarType::depends_on_tvar(string x, Tenv tenv) {
    // If the names match, there is a loop
    if (name == x)
        return true;
    
    Type *T = this;
    while (isType<VarType>(T)) {
        auto S = tenv->get_tvar(x);
        if (S->toString() == x)
            return true;
        else
            T = S;
    }

    return T->depends_on_tvar(x, tenv);
}

bool VarType::isConstant(Tenv tenv) {
    auto T = tenv->get_tvar(name);
    if (isType<VarType>(T)) {
        if (T->toString() == name)
            // It is its own reduction. Therefore it could be anything, and is non-constant.
            return false;
        else
            // It reduces to another type that can also be checked.
            return T->isConstant(tenv);
    } else
        // It is reducible, therefore see if that type is reducible.
        return T->isConstant(tenv);
}

Type* VarType::simplify(Tenv tenv) {
    // Get the known value
    Type *T = tenv->get_tvar(name);

    // If we have some idea, we can simplify.
    if (T->toString() != name) {
        T = T->simplify(tenv);
    
        // Set the new variable
        tenv->set_tvar(name, T);
    }
    
    // Return a copy
    return T->clone();
}
Type* AndExp::typeOf(Tenv tenv) {
    // Type the left
    auto X = left->typeOf(tenv);
    if (!X) return NULL;
    
    // Type the right
    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        return NULL;
    }
    
    auto B = new BoolType;
    
    // The left side is a boolean.
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete Y;
        delete B;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The right side is also a boolean
    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        delete B;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // We know the type, thus the types are not needed.
    delete x;
    delete y;

    show_proof_therefore(type_res_str(tenv, this, B));
    
    // And types to a boolean
    return B;
}

Type* AdtExp::typeOf(Tenv tenv) {
    // This expression is used as a backend expression. If it
    // needs to be typed by hand, something went wrong.
    show_proof_step("back-end ADT definition is not covered by typing system.");
    show_proof_therefore(type_res_str(tenv, this, NULL));
    return NULL;
}
Type* AdtDeclarationExp::typeOf(Tenv tenv) {
    // Given that the BNF parser was able to generate, it should be possible
    // to generate it. Hence, this expression will type.
    
    // Determine the number of subtypes
    int i;
    for (i = 0; argss[i]; i++);

    // Create a dictionary type
    auto bin = new DictType;
    auto fns = bin->getTypes();

    // The types of each argument of each subtype
    Type ***typess = new Type**[i+1];

    // The names of each kind
    string *kinds = new string[i+1];

    typess[i] = NULL;
    kinds[i] = "";

    while (i--) {
        // Transfer the name
        kinds[i] = ids[i];

        int j;
        for (j = 0; argss[i][j]; j++);

        // Pass on the argument types
        typess[i] = new Type*[j+1];
        typess[i][j] = NULL;

        while (j--)
            // Assign the argument element
            typess[i][j] = argss[i][j]->clone();
    }
    
    // We can now generate the ADT.
    Type *adt = new AlgebraicDataType(name, kinds, typess);
    
    for (i = 0; typess[i]; i++) {
        // Each element of the dictionary is a function that generates a subtype of the ADT.
        Type *fn = i ? adt->clone() : adt;
        if (!typess[i][0])
            fn = new LambdaType("", new VoidType, fn);
        else {
            int j;
            for (j = 0; typess[i][j]; j++);
            while (j--) fn = new LambdaType("arg" + to_string(j), typess[i][j]->clone(), fn);
        }
        
        // Add to the pool
        fns->add(ids[i], fn);
    }

    // Now, we can apply to the environment and determine the result.
    tenv->set(name, bin);

    // Evaluate the body
    auto T = body->typeOf(tenv);

    tenv->remove(name);

    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    return T;
}
Type* SwitchExp::typeOf(Tenv tenv) {
    // Type the given ADT
    auto A = adt->typeOf(tenv);
    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    } else if (isType<AlgebraicDataType>(A)) {
    } else if (isType<VarType>(A)) {
        // Because we do not know about the type of our expression,
        // we must determine a best guess.
        int i;
        for (i = 0; bodies[i]; i++);

        string *kinds = new string[i+1]; kinds[i] = "";
        Type ***argss = new Type**[i+1]; argss[i] = NULL;
        
        while (i--) {
            // Use the name in the slot
            kinds[i] = names[i];

            int j;
            for (j = 0; idss[i][j] != ""; j++);
            
            // Create a list of arbitrary types
            argss[i] = new Type*[j+1];
            argss[i][j] = NULL;
            while (j--) argss[i][j] = tenv->make_tvar();
        }
        
        // Construct our hypothesis
        auto Adt = new AlgebraicDataType("", kinds, argss);
        
        // Unify the two hypotheses
        auto T = A->unify(Adt, tenv);
        delete A;
        delete Adt;
        if (!T) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else
            A = T;
    } else {
        // The item is of some other type.
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *T = NULL;

    // Now, we can check each type
    for (int i = 0; idss[i]; i++) {
        // Add the argtypes to the tenv
        for (int j = 0; idss[i][j] != ""; j++)
            tenv->set(idss[i][j], ((AlgebraicDataType*) A)->getArgss()[i][j]->clone());
        
        // Type the body
        auto t = bodies[i]->typeOf(tenv);
        if (!t) {
            delete T;
            break;
        }
        
        // Unify the branches
        if (i) {
            auto S = T->unify(t, tenv);
            delete T;
            delete t;
            if (!S) break;
            else T = S;
        } else
            T = t;
        
        // Remove the items to the tenv
        for (int j = 0; idss[i][j] != ""; j++)
            tenv->remove(idss[i][j]);
    }
    
    delete A;

    show_proof_therefore(type_res_str(tenv, this, T));
    return T;

}
Type* ApplyExp::typeOf(Tenv tenv) {
    show_proof_step("To type " + toString() + ", we must match the parameter type(s) of the function with its arguments.");
    auto T = op->typeOf(tenv);
    
    auto Tmp = T->simplify(tenv);
    delete T;
    T = Tmp;

    show_proof_step("Thus, the function " + op->toString() + " in function call " + toString() + " has type " + T->toString() + " under " + tenv->toString() + ".");

    if (isType<VarType>(T)) {
        // We need to create a unification that indicates that
        // the variable is a function.
        
        if (!args[0]) {
            // The function should type as a void -> t
            auto Y = tenv->make_tvar();
            auto F = new LambdaType("", new VoidType, Y);
            auto G = T->unify(F, tenv);
            delete T;
            delete F;
            delete Y;

            return G;
        }
        
        // Define the number of arguments in the function call.
        int argc;
        for (argc = 0; args[argc]; argc++);
        
        // We will define the types of the arguments in order to create a
        // hypothesis about the type of the currently untyped function.
        Type **xs = new Type*[argc];
        for (int i = 0; args[i]; i++) {
            auto X = args[i]->typeOf(tenv);
            if (!X) {
                while (i--) delete xs[i];
                delete[] xs;
                show_proof_therefore(type_res_str(tenv, this, NULL));
                return NULL;
            } else {
                show_proof_step("Knowing " + type_res_str(tenv, args[i], X) + ", we are closer to inferring the type of " + op->toString() + ".");
                xs[i] = X;
            }
        }
        
        // We suppose that our function outputs something of some type Y
        auto Y = tenv->make_tvar();
        
        // Build the lambda type, starting from some arbitrary
        // body type that could be decided later.
        auto F = Y->clone();
        show_proof_step("We will use " + F->toString() + " to define the currently arbitrary return type of " + op->toString() + ", which takes " + to_string(argc) + " arguments.");
        while (argc--) {
            show_proof_step("We know that " + type_res_str(tenv, args[argc], xs[argc]) + ".");
            F = new LambdaType("", xs[argc], F);
        }
        
        // Unification of the function type with our hypothesis
        auto S = T->unify(F, tenv);
        
        // GC
        delete T;
        delete F;
        delete[] xs;

        if (!S) {
            delete Y;
            Y = NULL;
        }
         
        show_proof_therefore(type_res_str(tenv, this, Y));
        return Y;

    } else if (isType<LambdaType>(T)) {
        if (!args[0]) {
            // We expect the function to take void and return whatever the
            // right hand side yields.
            if (!isType<VoidType>(((LambdaType*) T)->getLeft())) {
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete T;
                return NULL;
            }
            
            // Thus, our hypothesis should hold.
            show_proof_therefore(type_res_str(tenv, this, ((LambdaType*) T)->getRight()));
            return ((LambdaType*) T)->getRight()->clone();
        }

        show_proof_step("We must consolidate " + T->toString() + " under " + tenv->toString() + ".");
        
        for (int i = 0; args[i]; i++) {
            if (!isType<LambdaType>(T)) {
                // If we apply too many arguments, then we cannot actually
                // perform the application. Hence, we have detected an improper
                // function call.
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete T;
                return NULL;
            }
            auto F = (LambdaType*) T;
            
            // Type the argument
            auto X = args[i]->typeOf(tenv);
            
            if (!X) {
                // The argument is untypable
                delete T;
                show_proof_therefore(type_res_str(tenv, this, NULL));
                return NULL;
            }

            show_proof_step("In order to permit the types, we require subsumption "
                    + X->toString() + " <: " + F->getLeft()->toString() + " to hold under " + tenv->toString());
            
            // Subsumption requires that our type X can be used in any way
            // that the argument of F can be used (ex: Z as an R is okay)
            if (!X->equals(F->getLeft(), tenv)) {
                show_proof_step("The subsumption condition fails");
                show_proof_therefore(type_res_str(tenv, this, NULL));

                // Garbage collection
                delete F;
                delete X;
                return NULL;
            }
            
            // In the function tenv, we will unify the
            // argument types
            Type *Z;
            show_proof_step("To type " + toString()
                    + ", we must unify " + X->toString()
                    + " = " + F->getLeft()->toString() + " under "
                    + tenv->toString() + ".");
            
            // Because the subsumption holds, we can unify the two types.
            Z = X->unify(F->getLeft(), tenv);
            delete X;
            delete Z;

            if (!Z) {
                // Non-unifiable
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete F;
                return NULL;
            }
            
            // Thus, since X unifies w/ the argument of F as Z, the output
            // is based on reduction to the right side of T. We also
            // reduce it as far as possible.
            T = F->getRight()->simplify(tenv);
            delete F;
        }
        
        show_proof_therefore(type_res_str(tenv, this, T)); // QED

        // End case: return T
        return T;
    } else {
        show_proof_therefore(type_res_str(tenv, this, T));
        delete T;
        return NULL;
    }
}
Type* CastExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete T;

    T = type->clone();
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* CompareExp::typeOf(Tenv tenv) {
    auto A = left->typeOf(tenv);
    auto B = right->typeOf(tenv);

    auto C = A->unify(B, tenv);
    delete A;
    delete B;

    if (C) {
        delete C;
        C = new BoolType;
    }

    show_proof_therefore(type_res_str(tenv, this, C));
    return C;

}
/*
C |- x : s    C |- M : t
------------------------
   C |- d/dx M : dt/ds
*/
Type* DerivativeExp::typeOf(Tenv tenv) {
    auto Y = func->typeOf(tenv);

    if (Y) {
        Type *X;

        if (!isType<LambdaType>(Y)) {
            X = tenv->apply(var);
        } else {
            show_proof_step("We must extract type of arg " + var + " from " + Y->toString());
            Type *F = Y;
            while (isType<LambdaType>(F) && ((LambdaType*) F)->getX() != var)
                F = ((LambdaType*) F)->getRight();

            show_proof_step("Search reduces to subtype " + F->toString());

            if (isType<LambdaType>(F)) {
                X = ((LambdaType*) F)->getLeft()->clone();
                show_proof_step("Thus, yields " + var + " : " + X->toString());
            } else {
                show_proof_therefore(type_res_str(tenv, this, NULL));
                delete Y;
                return NULL;
            }
        }

        auto S = new DerivativeType(Y, X);
        
        show_proof_step("Thus, " + type_res_str(tenv, this, S) + " if it simplifies");

        auto T = S->simplify(tenv);
        delete S;

        show_proof_therefore(type_res_str(tenv, this, T));
        return T;
    } else {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

}
Type* DictExp::typeOf(Tenv tenv) {
    auto trie = new HashMap<std::string, Type*>;

    auto T = new DictType(trie);
    auto kt = keys->iterator();
    auto vt = vals->iterator();
    while (kt->hasNext()) {
        auto k = kt->next();
        auto v = vt->next();

        auto t = v->typeOf(tenv);
        if (!t) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            delete T;
            delete kt;
            delete vt;
            return NULL;
        } else
            trie->add(k, t);
    }
    delete kt;
    delete vt;

    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* DictAccessExp::typeOf(Tenv tenv) {
    Type *D = list->typeOf(tenv);
    
    Type *E = new DictType(new HashMap<std::string, Type*>);
    
    // The left hand side should be a dictionary.
    Type *F = D->unify(E, tenv);
    delete D;
    delete E;

    if (!F) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // Given the string, that type should be accessible.
    Type *T;
    if (((DictType*) F)->getTypes()->hasKey(idx))
        T = ((DictType*) F)->getTypes()->get(idx)->clone();
    else
        // If it doesn't exist, we imagine that it is possibly being created.
        T = tenv->make_tvar();

    delete F;

    show_proof_therefore(type_res_str(tenv, this, T));

    return T;
}


Type* FoldExp::typeOf(Tenv tenv) {
    /* Typing requires the following:
    list : [a]
    base : b
    func : b -> a -> b
    */
    
    // The list should be typable.
    Type *L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // That type should be some list type.
    Type *a;
    if (isType<ListType>(L))
        a = ((ListType*) L)->subtype()->clone();
    else if (isType<VarType>(L)) {
        auto M = new ListType(tenv->make_tvar());
        auto N = L->unify(M, tenv);
        delete M;
        if (N) {
            a = ((ListType*) N)->subtype()->clone();
            delete N;
        } else
            a = NULL;
    } else
        a = NULL;

    delete L;
    if (!a) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The base case should have some type.
    Type *b = base->typeOf(tenv);
    if (!b) {
        delete a;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The function should have some type.
    Type *F = func->typeOf(tenv);
    if (!F) {
        delete a;
        delete b;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    // The function should be a function that takes two arguments and returns some type.
    auto c = tenv->make_tvar();
    Type *G = new LambdaType("", b->clone(), new LambdaType("", a, c->clone()));
    Type *H = F->unify(G, tenv);
    
    delete F;
    delete G;

    if (H) {
        delete H;
        auto T = b->simplify(tenv);
        b = T;
    } else {
        delete b;
        delete c;
        b = NULL;

        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The base type and the function's output type should be unifiable.
    H = c->unify(b, tenv);

    delete b;
    delete c;

    show_proof_therefore(type_res_str(tenv, this, H));

    return H;
}
Type* ForExp::typeOf(Tenv tenv) {
    auto T = set->typeOf(tenv);
    if (!T) return NULL;

    auto L = new ListType(tenv->make_tvar());
    
    auto S = T->unify(L, tenv);
    delete T;
    delete L;

    if (!S) return NULL;
    T = ((ListType*) S)->subtype()->clone();
    delete S;
    
    show_proof_step("Thus, in the for body " + body->toString() + ", " + id + " : " + T->toString() + ".");
    show_proof_step("Let " + id + " : " + T->toString() + ".");
    
    // Remove the variable name from scope.
    Type *X = tenv->hasVar(id)
        ? tenv->apply(id)
        : NULL;

    // Add the var
    tenv->set(id, T);
    
    // Evaluate the body type
    S = body->typeOf(tenv);

    // Clean out the types
    if (X)
        tenv->set(id, X);
    else
        tenv->remove(id);
    
    // We now know the answer
    if (S) {
        delete S;
        S = new VoidType;
    }

    show_proof_therefore(type_res_str(tenv, this, S));
    return S;
}
Type* HasExp::typeOf(Tenv tenv) {
    // The element should be typable.
    auto X = item->typeOf(tenv);
    if (!X) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The structure should be typable.
    auto Xs = set->typeOf(tenv);
    if (!Xs) {
        delete X;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The structure should be a list.
    if (isType<ListType>(Xs)) {
        // The type of the list's contents should unify with the element's type.
        auto Y = X->unify(((ListType*) Xs)->subtype(), tenv);
        delete X;
        delete Xs;
        if (Y) {
            X = new BoolType;
            delete Y;
        } else
            X = NULL;
    } else {
        // The structure should be known to be a list.
        auto V = (VarType*) X;
        auto L = new ListType(X);
        X = Xs->unify(L, tenv);
        delete L;
        delete V;
        if (X) {
            // If the unification is possible, we can check membership.
            delete X;
            X = new BoolType;
        } else
            // If not a list, then it is not possible.
            X = NULL;
    }

    show_proof_therefore(type_res_str(tenv, this, X));
    return X;
}
Type* IfExp::typeOf(Tenv tenv) {
    // The conditional is a boolean
    auto C = cond->typeOf(tenv);
    if (!C) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = new BoolType;
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete D;

    auto T = tExp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto F = fExp->typeOf(tenv);
    if (!T) {
        delete T;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // The two branches should match types.
    auto Y = T->unify(F, tenv);
    delete T;
    delete F;

    show_proof_therefore(type_res_str(tenv, this, Y));

    return Y;
}
Type* ImportExp::typeOf(Tenv tenv) {

    // First, attempt to chack for a stdlib
    Type *T = type_stdlib(module);

    if (!T) {
        // Attempt to open the file
        ifstream file;
        file.open("./" + module + ".lom");
        
        // Ensure that the file exists
        if (!file) {
            throw_err("IO", "could not load module " + module);
            return NULL;
        }
        
        // Read the program from the input file
        int i = 0;
        string program = "";
        do {
            string s;
            getline(file, s);

            if (i++) program += " ";
            program += s;
        } while (file);

        throw_debug("module IO", "module " + module + " is defined by '" + program + "'\n");
        file.close();
        
        Exp modexp = parse_program(program);

        if (!modexp) {
            show_proof_therefore(type_res_str(tenv,this,NULL));
            return NULL;
        }

        T = modexp->typeOf(tenv);
        delete modexp;

        if (!T) {
            show_proof_therefore(type_res_str(tenv,this,NULL));
            return NULL;
        }
    }
    
    // Store the type
    tenv->set(name, T);
    
    // Evaluate the body
    Type *B = exp->typeOf(tenv);
    
    // Cleanup
    tenv->remove(name);
    
    // End of computation
    show_proof_therefore(type_res_str(tenv,this,B));
    return B;

}
Type* IsaExp::typeOf(Tenv tenv) {
    // The item should be typable, otherwise we can't verify its type.
    auto T = exp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    delete T;

    T = new BoolType;
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* LambdaExp::typeOf(Tenv tenv) {
    int i;
    for (i = 0; xs[i] != ""; i++);
    
    int argc = i;
    auto Ts = new Type*[argc];
    
    for (i = 0; i < argc; i++) {
        // Add it to the parameter arg list
        Ts[i] = tenv->make_tvar();
    }

    // We need to evaluate the type of the
    // body. This will also allow us to define
    // restrictions on the domain of the function.
    unordered_map<string,Type*> tmp;
    
    // Temporarily modify the environment
    for (i = 0; i < argc; i++) {
        if (tenv->hasVar(xs[i])) {
            auto t = tenv->apply(xs[i]);
            tmp[xs[i]] = t;
        }
        tenv->set(xs[i], Ts[i]->clone());
    }
    
    // Type the body
    auto T = exp->typeOf(tenv);

    if (!T) {
        // The body could not be typed.
        while (argc--)
            delete Ts[argc];
        delete[] Ts;
        return NULL;
    } else if (isType<VarType>(T)) {
        // Simplify the body to be the value of the
        // variable, if it is known.
        auto V = tenv->get_tvar(T->toString())->clone();
        show_proof_step("Hence, the type of body expression " + exp->toString() + " is that of variable " + T->toString() + " = " + V->toString() + " under " + tenv->toString() + ".");
        delete T;
        T = V;
    }

    if (argc == 0) {
        T = new LambdaType("", new VoidType, T);
    } else {
        // Reset the environment
        for (int i = 0; i < argc; i++)
            // Remove the stuff
            tenv->remove(xs[i]);
        for (auto it : tmp)
            // Put the old stuff in
            tenv->set(it.first, it.second);

        for (i = argc - 1; i >= 0; i--) {
            T = new LambdaType(xs[i], tenv->get_tvar(Ts[i]->toString())->simplify(tenv), T);
            delete Ts[i];
        }
    }


    delete[] Ts;

    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    return T;
}
Type* ListExp::typeOf(Tenv tenv) {
    // If the list is empty, it could contain any type
    if (size() == 0)
        return new ListType(tenv->make_tvar());
    
    // Get the type of the first element
    auto T = get(0)->typeOf(tenv);
    
    // Next, we must verify that the type of each element is unifiable.
    for (int i = 1; T && i < size(); i++) {
        auto A = get(i)->typeOf(tenv);
        if (!A) {
            delete T;
            T = NULL;
            break;
        }

        auto B = T->unify(A, tenv);
        delete T;
        delete A;

        T = B;
    }
    
    if (T) {
        T = new ListType(T);
        show_proof_therefore(type_res_str(tenv, this, T));
        return T;
    } else {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
}
Type* ListAccessExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Ts = new ListType(tenv->make_tvar());

    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto I = idx->typeOf(tenv);
    if (!I) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Z = new IntType;

    // The type of the index must be unifiable w/ Z
    auto B = I->unify(Z, tenv);
    delete I;
    delete Z;
    if (!B) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    auto T = ((ListType*) A)->subtype()->clone();

    delete A;
    delete B;

    return T;
}
Type* ListAddExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto E = elem->typeOf(tenv);
    if (!E) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete L;
        return NULL;
    }

    auto Ts = new ListType(E);

    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto I = idx->typeOf(tenv);
    if (!I) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Z = new IntType;

    // The type of the index must be unifiable w/ Z
    auto B = I->unify(Z, tenv);
    delete I;
    delete Z;
    if (!B) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    auto T = ((ListType*) A)->subtype()->clone();

    delete A;
    delete B;

    return T;
}
Type* ListRemExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Ts = new ListType(tenv->make_tvar());

    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto I = idx->typeOf(tenv);
    if (!I) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Z = new IntType;

    // The type of the index must be unifiable w/ Z
    auto B = I->unify(Z, tenv);
    delete I;
    delete Z;
    if (!B) {
        delete A;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    auto T = ((ListType*) A)->subtype()->clone();

    delete A;
    delete B;

    return T;
}
Type* ListSliceExp::typeOf(Tenv tenv) {
    auto L = list->typeOf(tenv);
    if (!L) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto Ts = new ListType(tenv->make_tvar());
    
    auto A = L->unify(Ts, tenv);
    delete L;
    delete Ts;

    if (!A)
        return NULL;
    
    // Typing of the first index
    Type *I = NULL;
    Type *Z = NULL;
    
    if (from) {
        I = from->typeOf(tenv);
        if (!I) {
            delete A;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }

        Z = new IntType;

        // The type of the from index must be unifiable w/ Z
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else delete B;
    }
    
    // Now, we type the second index if it is given.
    if (to) {
        I = to->typeOf(tenv);
        if (!I) {
            delete A;
            delete Z;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
        Z = new IntType;
        auto B = I->unify(Z, tenv);
        delete I;
        delete Z;

        if (!B) {
            delete A;
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        }
    } else
        delete Z;

    show_proof_therefore(type_res_str(tenv, this, A));
    return A;
}
Type* MagnitudeExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    
    Type *Y;
    if (T) {
        if (isType<IntType>(T)
        ||  isType<ListType>(T)
        ||  isType<StringType>(T)
        ||  isType<BoolType>(T)
        )
            Y = new IntType;
        else if (isType<RealType>(T)
             ||  isType<VarType>(T))
            Y = new RealType;
        else
            Y = NULL;
        
        delete T;
    } else
        Y = NULL;

    show_proof_therefore(type_res_str(tenv, this, Y));
    
    return Y;
}
Type* MapExp::typeOf(Tenv tenv) {
    /* To type a map, we require that:
    1) list : [a] for some t
    2) func : a -> b
    Then, map : [b]
    */
    Type *F = func->typeOf(tenv);
    if (!F) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    Type *L = list->typeOf(tenv);
    if (!L) {
        delete F;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    show_proof_step("To type the map, we let the function have type "
            + F->toString() + " and the list have type " + L->toString() + ".");
    
    // Compute the type of the source type
    Type *a = NULL;
    if (isType<ListType>(L)) {
        a = ((ListType*) L)->subtype()->clone();
    } else if (isType<VarType>(L)) {
        Type *t = new ListType(tenv->make_tvar());
        Type *u = L->unify(t, tenv);
        delete t;

        if (u) {
            a = ((ListType*) u)->subtype()->clone();
            delete u;
        }
    }
    
    // If we can't find a, then we're screwed.
    delete L;
    if (!a) {
        delete F;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    // Next, we need to verify the functionness of F, and
    // go from there.
    Type *t = new LambdaType("", a->clone(), tenv->make_tvar());
    show_proof_step("We require that " + F->toString() + " and " + t->toString() + " unify under " + tenv->toString() + ".");
    Type *u = F->unify(t, tenv);
    delete t;
    
    Type *b = NULL;
    if (u) {
        b = ((LambdaType*) u)->getRight()->clone();
        delete u;
    }

    delete F;
    delete a;
    
    // Final simplifications
    if (b) {
        a = b->simplify(tenv);
        delete b;
        b = a;
    }

    L = b ? new ListType(b) : NULL;

    show_proof_therefore(type_res_str(tenv, this, L));

    return L;
}

Type* NormExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, T));
        return NULL;
    } else if (T->isConstant(tenv)) {
        // We can check numericality
        Type *R = T;
        int d = 0;
        while (isType<ListType>(R) && ++d <= 2)
            R = ((ListType*) R)->subtype();
        
        // Ban normalization of 3D or higher lists. We also verify sums.
        if (d > 2 || (!isType<RealType>(R) && !isType<IntType>(R)))
            R = NULL;
        else
            R = new RealType;
        
        delete T;

        show_proof_therefore(type_res_str(tenv, this, R));
        return NULL;
    } else {
        T = new MultType(T, tenv->make_tvar());
        auto U = T->simplify(tenv);
        delete T;

        show_proof_therefore(type_res_str(tenv, this, U));
        return U;
    }
}
Type* NotExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (!T) return NULL;

    auto B = new BoolType;

    auto x = T->unify(B, tenv);
    delete T;
    if (!x) {
        delete B;
        B = NULL;
    }
    
    delete x;
    
    show_proof_therefore(type_res_str(tenv, this, B));

    return B;

}
Type* OrExp::typeOf(Tenv tenv) {
    auto X = left->typeOf(tenv);
    if (!X) return NULL;

    auto Y = right->typeOf(tenv);
    if (!Y) {
        delete X;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto B = new BoolType;
    
    auto x = X->unify(B, tenv);
    delete X;
    if (!x) {
        delete B;
        delete Y;
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }

    auto y = Y->unify(B, tenv);
    delete Y;
    if (!y) {
        delete x;
        delete B;
        return NULL;
    }
    
    delete x;
    delete y;

    show_proof_therefore(type_res_str(tenv, this, B));

    return B;
}
Type* PrintExp::typeOf(Tenv tenv) {
    for (int i = 0; args[i]; i++) {
        Type *T = args[i]->typeOf(tenv);
        if (!T) {
            show_proof_therefore(type_res_str(tenv, this, NULL));
            return NULL;
        } else
            delete T;
    }
    
    Type *T = new VoidType;
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* SequenceExp::typeOf(Tenv tenv) {
    Type *T = NULL;

    auto it = seq->iterator();
    do {
        if (T) delete T;
        T = it->next()->typeOf(tenv);
    } while (it->hasNext() && T);

    delete it;

    show_proof_therefore(type_res_str(tenv, this, T));

    return T;
}
Type* SetExp::typeOf(Tenv tenv) {
    // The target and expression should each have a type
    auto T = tgt->typeOf(tenv);
    if (!T) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    auto E = exp->typeOf(tenv);
    if (!E) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        delete T;
        return NULL;
    }

    show_proof_step("Typing requires assignment: " + T->toString() + " = " + E->toString());

    Type *S = E->equals(T, tenv)
        ? T->unify(E, tenv)
        : NULL;

    delete E;
    delete T;

    if (S && isExp<VarExp>(tgt)) {
        show_proof_step(tgt->toString() + " is a variable, hence we generalize its type to " + S->toString());
        tenv->set(tgt->toString(), S->clone());
    }
    
    show_proof_therefore(type_res_str(tenv, this, S));
    
    return S;
}
Type* LetExp::typeOf(Tenv tenv) {
    unordered_map<string, Type*> tmp;
    
    int i;
    for (i = 0; exps[i]; i++) {
        show_proof_step("Let value " + ids[i] + " = " + exps[i]->toString()); // Initial condition.

        // We seek to define the type of the ith variable to be defined.
        auto t = exps[i]->typeOf(tenv);
    
        // Answer to typability
        show_proof_step("Therefore, " + tenv->toString() + " ⊢ " + ids[i] + (t ? (" : " + t->toString()) : " is untypable"));

        if (!t) {
            break;
        } else {
            if (tenv->hasVar(ids[i]))
                // We may have to suppress variables.
                tmp[ids[i]] = tenv->apply(ids[i]);

            tenv->set(ids[i], t);

        }
    }

    // Evaluate the body if possible
    auto T = exps[i] ? NULL : body->typeOf(tenv);

    // Restore the tenv
    for (i = 0; exps[i]; i++)
        tenv->remove(ids[i]);
    for (auto it : tmp) {
        tenv->set(it.first, it.second);
        tmp[it.first] = NULL;
    }
    
    show_proof_therefore(type_res_str(tenv, this, T)); // QED

    return T;
}
Type* StdMathExp::typeOf(Tenv tenv) {
    auto T = e->typeOf(tenv);
    if (!T) return NULL;
    
    switch (fn) {
        // Certain functions take lists and give numbers
        case MIN:
        case MAX: {
            // The input must be a ist of numbers
            auto L = new ListType(new RealType);
            auto U = T->unify(L, tenv);
            delete T;
            delete L;
            
            // The output type is the same as that of the contents.
            L = (ListType*) U;
            U = L->subtype()->clone();
            delete L;

            return U;

        } 
        // Others will take numbers and give new numbers
        default: {
            auto R = new RealType;

            auto U = T->unify(R, tenv);
            delete T;
            delete R;

            return U;
        }
    }
}
Type* TupleAccessExp::typeOf(Tenv tenv) {
    auto T = exp->typeOf(tenv);
    if (T) {
    if (isType<TupleType>(T)) {
        auto TT = (TupleType*) T;
        T = (idx ? TT->getRight() : TT->getLeft())->clone();
        delete TT;
    } else if (isType<VarType>(T)) {
        // Simplify and toss out 
        auto TT = new TupleType(tenv->make_tvar(), tenv->make_tvar());
        auto t = T->unify(TT, tenv);
        delete T;
        delete TT;

        if (t) {
            T = (idx ?
                ((TupleType*) t)->getRight()
                : ((TupleType*) t)->getLeft())->clone();
            delete t;
        } else
            T = NULL;
    } else {
        delete T;
        T = NULL;
    }
    }
    
    // End result
    show_proof_therefore(type_res_str(tenv, this, T));
    return T;
}
Type* WhileExp::typeOf(Tenv tenv) {
    auto C = cond->typeOf(tenv);
    auto B = new BoolType;

    if (!C) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    
    // Type the conditional
    auto D = C->unify(B, tenv);
    delete B;
    delete C;

    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    else delete D;
    
    // Type the body
    D = body->typeOf(tenv);
    if (!D) {
        show_proof_therefore(type_res_str(tenv, this, NULL));
        return NULL;
    }
    else delete D;

    D = new VoidType;
    show_proof_therefore(type_res_str(tenv, this, D));
    return D;
}


