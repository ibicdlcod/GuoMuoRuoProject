#ifndef WCWIDTH_H
#define WCWIDTH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

extern int mk_wcwidth(wchar_t ucs);
extern int mk_wcswidth(const wchar_t *pwcs, size_t n);

#ifdef __cplusplus
}
#endif

#endif // WCWIDTH_H
