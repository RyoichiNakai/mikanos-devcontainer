#include <errno.h>
#include <sys/types.h>

caddr_t sbrk(int incr) {
  errno = ENOMEM;
  return NULL;
}
