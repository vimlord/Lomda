#include "stdlib.hpp"
#include "expressions/stdlib.hpp"

#include "expression.hpp"

inline void swap(Val *vs, int i, int j) {
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

        /**

        2 1 3 5 4
            |
        */
        
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
        throw_err("type", "sort.mergesort : [R] -> void applied to invalid argument " + A->toString());
        return NULL;
    }

    List<Val> *L = ((ListVal*) A)->get();

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

Type* type_stdlib_sort() {
    return new DictType {
        {
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

