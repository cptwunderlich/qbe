#include <unistd.h>
#include <fcntl.h>

long get_random_canary(void) {
  long res;
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0) {
    ssize_t reslen = read(fd, &res, sizeof(res));
    if (reslen == (ssize_t) sizeof(res))
      return res;
  }

  return 0xff0a0000;
}

