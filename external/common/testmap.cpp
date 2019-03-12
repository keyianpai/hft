#include <iostream>
#include <tr1/unordered_map>
#include <string>
#include "include/common_tools.h"

using namespace std;

int main() {
  std::tr1::unordered_map<std::string, std::string> a;
  a["asdas"] = "231123";
  a["a"] = "2";
  cout << MapString(a, "a");
}
