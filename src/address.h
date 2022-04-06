#ifndef __ADDRESS_H_
#define __ADDRESS_H_

#include <netinet/in.h> /* * sockaddr_in6 */
#include <stdbool.h>
#include <sys/types.h> /* sockaddr_in6 */

bool human_readable_ip(char* buffer, const struct sockaddr_storage* addr);

void print_ip(const char* addr_name, const struct sockaddr_storage* addr);

/* Resolve the resource name to an usable IP address
 * @address: The name to resolve
 * @rval: Where the resulting IP address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char* real_address(const char* address, struct sockaddr_storage* rval);

#endif
