#include "stdlib.hpp"
#include "expressions/stdlib.hpp"

#include "expression.hpp"

template<typename T>
inline void swap(T *vs, int i, int j) {
    auto tmp = vs[i];
    vs[i] = vs[j];
    vs[j] = tmp;
}

/**
 * Attempts to sort the array using the mergesort technique.
 * @param vs The values to sort
 * @param i The low end of the partition to sort (default: 0)
 * @param j The high end of the partition to sort (default: |vs|)
 * @return Whether or not the sort could properly terminate.
 */
bool mergesort(Val *vs, int i, int j) {
    static CompareExp compare(NULL, NULL, GT);

    if (j - i > 1) {
        // Sort each half, and if either fails, the whole
        // chain fails with it.
        int c = (i+j) / 2;
        if (!mergesort(vs, i, c) || !mergesort(vs, c, j))
            return false;

        // Then, merge
        Val *xs = new Val[j - i];
        int x;
        int a = i, b = c;
        for (x = 0; a < c && b < j; x++) {
            // Compute the side to favor
            BoolVal *B = (BoolVal*) compare.op(vs[b], vs[a]);
            if (!B) {
                // Comparison fails
                delete[] xs;
                return false;
            }

            if (B->get()) {
                // Transfer the element
                xs[x] = vs[a++];
            } else
                xs[x] = vs[b++];

            B->rem_ref();
        }

        // Finish the transfer
        while (a < c) xs[x++] = vs[a++];
        while (b < j) xs[x++] = vs[b++];
        
        // Transfer
        for (x = 0; x < j - i; x++)
            vs[x+i] = xs[x];

    }

    return true;
}

/**
 * Attempts to sort the array using the mergesort technique.
 * @param vs The values to sort
 * @param i The low end of the partition to sort (default: 0)
 * @param j The high end of the partition to sort (default: |vs|)
 * @return Whether or not the sort could properly terminate.
 */
bool quicksort(Val *vs, int i, int j) {
    static CompareExp compare(NULL, NULL, GT);

    if (j - i > 2) {
        BoolVal *B;
        
        // The value to partition over
        Val p = vs[i];
    
        // Extract a partition, and set it aside
        swap(vs, i, (i+j)/2);

        int lo = i;
        int hi = j;
        while (true) {
            while (++lo < hi) {
                B = (BoolVal*) compare.op(vs[lo], p);
                if (!B) return false;
                bool b = B->get();
                B->rem_ref();
                if (b) break;
            }
            while (--hi > lo) {
                B = (BoolVal*) compare.op(p, vs[hi]);
                if (!B) return false;
                bool b = B->get();
                B->rem_ref();
                if (b) break;
            }
            
            if (hi <= lo) {
                // Put the partition value into place
                swap(vs, i, hi);
                break;
            } else
                swap(vs, hi, lo);
        }

        // Thus, the partition is at hi

        quicksort(vs, i, hi-1);
        quicksort(vs, hi+1, j);


    } else if (j - i == 2) {
        // Check ordering
        BoolVal *B = (BoolVal*) compare.op(vs[i], vs[i+1]);

        // Swap if unsorted
        if (!B) return false;
        else if (B->get())
            swap(vs, i, i+1);
    }

    return true;
}

Val std_sort(Env env, bool (*sort)(Val*, int, int)) {
    Val A = env->apply("L");
    if (!isVal<ListVal>(A)) {
        // Illegal argument.
        throw_err("type", "sort.mergesort : [R] -> void cannot be applied to invalid argument " + A->toString());
        return NULL;
    }

    auto L = (ListVal*) A;

    // Build an array
    Val *vs = new Val[L->size()];
    for (int i = 0; i < L->size(); i++)
        vs[i] = L->get(i);
    
    // Sort the array
    bool res = sort(vs, 0, L->size());
    
    // Then, we put everything in
    for (int i = 0; i < L->size(); i++)
        L->set(i, vs[i]);

    return res ? new VoidVal : NULL;

}

Val std_mergesort(Env env) { return std_sort(env, mergesort); }
Val std_quicksort(Env env) { return std_sort(env, quicksort); }

Val std_is_sorted(Env env) {
    Val L = env->apply("L");
    if (!isVal<ListVal>(L)) {
        throw_err("type", "sort.is_sorted : [R] -> B cannot be applied to invalid argument " + L->toString());
        return NULL;
    }

    CompareExp comp(NULL, NULL, CompOp::GT);

    ListVal *list = (ListVal*) L;
    if (list->size() < 2) return new BoolVal(true);

    auto it = list->iterator();
    Val v = it->next();
    while (it->hasNext()) {
        Val u = it->next();

        BoolVal *b = (BoolVal*) comp.op(u, v);
        if (!b) {
            delete it;
            return NULL;
        } else if (!b->get()) {
            delete it;
            return new BoolVal(false);
        }
    }

    delete it;
    return new BoolVal(true);

}

Type* type_stdlib_sort() {
    return new DictType {
        {
            "is_sorted",
            new LambdaType("L",
                new ListType(new RealType), new BoolType)
        }, {
            "mergesort",
            new LambdaType("L",
                new ListType(new RealType), new VoidType)
        },
        {
            "quicksort",
            new LambdaType("L",
                new ListType(new RealType), new VoidType)
        }
    };
}

Val load_stdlib_sort() {
    return new DictVal {
        {
            "is_sorted",
            new LambdaVal(new std::string[2]{"L", ""},
                new ImplementExp(std_is_sorted, NULL))
        },
        {
            "mergesort",
            new LambdaVal(new std::string[2]{"L", ""},
                new ImplementExp(std_mergesort, NULL))
        },
        {
            "quicksort",
            new LambdaVal(new std::string[2]{"L", ""},
                new ImplementExp(std_quicksort, NULL))
        }
    };
}

