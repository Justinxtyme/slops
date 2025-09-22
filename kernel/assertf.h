#ifndef ASSERTF_H
#define ASSERTF_H

void assert_fail(const char *expr, const char *file, int line);

#define assertf(expr) \
    ((expr) ? (void)0 : assert_fail(#expr, __FILE__, __LINE__))


#endif