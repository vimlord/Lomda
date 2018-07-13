#include "parser.hpp"

using namespace std;

result<Type> parse_type(string str) {
    result<Type> res;
    res.value = NULL;
    res.strlen = -1;

    int i;
    if ((i = starts_with(str, "Z")) > 0)
        res.value = new IntType;
    else if ((i = starts_with(str, "R")) > 0)
        res.value = new RealType;
    else if ((i = starts_with(str, "S")) > 0)
        res.value = new StringType;
    else if ((i = starts_with(str, "V")) > 0)
        res.value = new VoidType;
    else if ((i = starts_with(str, "ADT")) > 0) {
        int j;

        // The opening brace
        if ((j = starts_with(str.substr(i), "<")) == -1) {
            // No type could be found
            return res;
        } else
            i += j;
        
        // The name of the ADT
        string id = extract_identifier(str.substr(i));
        if (id == "") {
            // No type could be found
            return res;
        } else
            i += id.length();
        
        // The closing brace
        if ((j = starts_with(str.substr(i), ">")) == -1) {
            // No type could be found
            return res;
        } else {
            // We can now define the type
            i += j;
            res.value = new AlgebraicDataType(id);
        }

    } else if ((i = starts_with(str, "(")) > 0) {
        // Encapsulate a type in parentheses.
        i = index_of_char(str, '(');
        int j = index_of_closure(str.substr(i), '(', ')');
        
        if (j == -1)
            return res;

        res = parse_type(str.substr(i+1, j-1));
        if (!res.value) {
            res.reset();
            return res;
        } else
            i += j+1;
    } else if ((i = starts_with(str, "[")) > 0) {
        i = index_of_char(str, '[');
        int j = index_of_closure(str.substr(i), '[', ']');

        if (j == -1)
            return res;

        res = parse_type(str.substr(i+1, j-1));
        if (!res.value) {
            res.reset();
            return res;
        } else {
            res.value = new ListType(res.value);
            i += j+1;
        }
        
    } else {
        // No type could be found
        return res;
    }

    // Progress the string
    str = str.substr(i);

    // Adjust the length of the string to compensate.
    res.strlen = i;

    // Now, we will check to see if there is more.
    if ((i = starts_with(str, "->")) > 0) {
        result<Type> alt = parse_type(str.substr(i));
        if (!alt.value)
            res.reset();
        else {
            res.value = new LambdaType(res.value, alt.value);
            res.strlen += i + alt.strlen;
        }
    } else if ((i = starts_with(str, "*")) > 0) {
        result<Type> alt = parse_type(str.substr(i));
        if (!alt.value)
            res.reset();
        else {
            res.value = new TupleType(res.value, alt.value);
            res.strlen += i + alt.strlen;
        }
    }

    return res;

}

