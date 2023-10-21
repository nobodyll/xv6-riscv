#include "user/user.h"
#include "kernel/types.h"

int main() {
  int pid;
  char c;

  pid = fork();
  if (pid == 0) {
    c = '/';
  } else {
    c = '\\';
  }

  for (int i = 0; ; i++) {
    if ((i % 1000000) == 0) {
      write(2, &c, 1);
    }
  }

  exit(0);
}