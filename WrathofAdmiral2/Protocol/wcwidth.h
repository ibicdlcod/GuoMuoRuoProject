#ifndef WCWIDTH_H
#define WCWIDTH_H

#include <QChar>

#include <wchar.h>

extern int mk_wcwidth(ushort ucs);
extern int mk_wcswidth(const QChar *pwcs, size_t n);

#endif // WCWIDTH_H
