#ifndef GLKUNIT_H
#define GLKUNIT_H

#include <stdio.h>

#define _BEGIN do {
#define _END } while(0);

/* msg must be a string literal */
#define _ASSERT(expr, msg, ...) _BEGIN \
    if( !(expr) ) { \
        fprintf(stderr, "Assertion failed: " msg "\n", __VA_ARGS__); \
        return 0; \
    } _END

#define SUCCEED _BEGIN return 1; _END
#define ASSERT(expr) _ASSERT(expr, "%s", #expr)
#define ASSERT_EQUAL(expected, actual) _ASSERT((expected) == (actual), "%s == %s", #expected, #actual);

struct TestDescription {
    char *name;
    int (*testfunc)(void);
};

#endif /* GLKUNIT_H */