#ifndef USEFUL_FUNCTION_H_
#define USEFUL_FUNCTION_H_

#include "define.h"
#include "order_side.h"
#include "order_action.h"
#include "order_status.h"
#include "offset.h"

#include <stdio.h>
#include <sys/time.h>

#include <string>
#include <vector>

inline std::vector<std::string> SplitStr(std::string str, std::string pattern) {
  std::string::size_type pos;
  std::vector<std::string> result;
  str = str + pattern;
  int size = str.size();
  for (int i = 0; i < size - 1; i++) {
      pos = str.find(pattern, i);
      int ps = static_cast<int>(pos);
      if (ps < size) {
            std::string s = str.substr(i, ps-i);
            result.push_back(s);
            i = ps + pattern.size() - 1;
          }
    }
  return result;
}

#endif  //  EXTERNAL_COMMON_USEFUL_FUNCTION_H_
