#include <stdio.h>
#include <time.h>
#include <iostream>

#include "simdata/datagener.h"

int main() {
  // DataGener dg("data", "/root/hft/data.log");
  clock_t start = clock();
  DataGener dg("data_source", "/root/data.log");
  dg.Run();
  clock_t end = clock();
  printf("time : %lf s\n", (static_cast<double>(end) - start) / CLOCKS_PER_SEC);
  return 0;
}
