#ifndef COMMON_TOOLS_H
#define COMMON_TOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <tr1/unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "market_snapshot.h"
using namespace std;

std::vector<std::string> Split(std::string raw_string, char split_char=' ');
bool CheckAddressLegal(std::string address);
bool CheckTimeStringLegal(std::string time);
void Register(std::tr1::unordered_map<std::string, int>* m, std::string contract, int no);
int Translate(std::string time);
std::string ExtractString(std::string s, char rm_char=' ');

template <class T1, class T2>
void PrintMap(std::tr1::unordered_map<T1, T2>map) {
  cout << "Start print map:";
  for (class std::tr1::unordered_map<T1, T2>::iterator it = map.begin(); it != map.end(); it++) {
    cout << "map[" << it->first << "]=" << it->second << " ";
  }
  cout << "End print map" << endl;
}

template <class T>
void PrintVector(std::vector<T>v) {
  cout << "Start print vector:\n";
  for (unsigned int i = 0; i < v.size(); i++) {
    cout << v[i] << "\n";
  }
  cout << "End print vector" << endl;
}
template <class T1, class T2>
std::string MapString(std::tr1::unordered_map<T1, T2>map, std::string id) {
  std::string head = "****************************";
  head +=  id;
  head += "head****************************\n";
  stringstream ss;
  for (class std::tr1::unordered_map<T1, T2>::iterator it = map.begin(); it != map.end(); it++) {
    ss << "map[" << it->first << "]=" << it->second << ";";
  }
  std::string content;
  ss >> content;
  std::string tail = "\n****************************";
  tail += id;
  tail += "tail****************************\n\n";
  return head + content + tail;
}

template <class T>
std::string VectorString(std::vector<T>v, std::string id) {
  std::string head = "****************************";
  head += id;
  head += " head****************************\n";
  stringstream ss;
  for (unsigned int i = 0; i < v.size(); i++) {
    ss << v[i] << ";";
  }
  std::string content;
  ss >> content;
  std::string tail = "\n****************************";
  tail += id;
  tail += " tail****************************\n\n";
  return head + content + tail;
}


template <class T>
bool CheckVSize(std::vector<T>v, int size) {
  int real_size = v.size();
  if (real_size == size) {
    return true;
  } else {
    // printf("check vector size failed, size if %d, require_size is %d\n", real_size, size);
    // PrintVector(v);
    return false;
  }
}

double PriceCorrector(double price, double min_price, bool is_upper = false);
bool DoubleEqual(double a, double b, double min_vaule = 0.0000001);
void SimpleHandle(int line);

MarketSnapshot HandleSnapshot(std::string raw_shot);

#endif // COMMON_TOOLS_H
