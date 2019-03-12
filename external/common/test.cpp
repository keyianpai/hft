#include "include/common_tools.h"


/*
template <class T1, class T2> 
void PrintMap(std::tr1::unordered_map<T1, T2>map) {
  cout << "Start print map:" << endl;
  for (auto it = map.begin(); it != map.end(); it++) {
    cout << "map[" << it->first << "]=" << it->second << ", ";
  }
  cout << "End print map" << endl;
}

template <class T>
void PrintVector(std::vector<T>v) {
  cout << "Start print vector:\n";
  for (int i = 0; i < v.size(); i++) {
    cout << v[i] << " ";
  }
  cout << "End print vector" << endl;
}
*/

int main() {
  /*
  std::tr1::unordered_map<int, int> v;
  v[1] = 1;
  v[10] = 10;
  PrintMap<int, int>(v);
  */
  std::string a = " 19309.01 0 8445877 M 0 0";
  cout << "head"<<ExtractString(a) <<"tail" <<  endl;
  std::vector<std::string> v = Split(a);
  PrintVector(v);
  cout << "asd" << ExtractString("0");
}
