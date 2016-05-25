#include <unistd.h>
#include <fcntl.h>

int get_random_canary(void) {
  int res;
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0) {
    ssize_t reslen = read(fd, &res, sizeof(res));
    if (reslen == (ssize_t) sizeof(res))
      return res;
  }

  return 0;
}

