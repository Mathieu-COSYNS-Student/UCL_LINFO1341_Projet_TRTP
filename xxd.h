#ifndef __XXD_H_
#define __XXD_H_

#include <stddef.h>

/**
 * Ouput content of addr in a readable hexadecimal format
 */
void hexDump(size_t offset, void* addr, int len);

#endif // !__XXD_H_