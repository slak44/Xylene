#include "utils/util.hpp"

std::vector<std::string> split(const std::string& str, char delim) {
  std::vector<std::string> vec {};
  std::stringstream ss(str);
  std::string item;
  while (getline(ss, item, delim)) vec.push_back(item);
  return vec;
}

std::string getAddressStringFrom(const void* ptr) {
  std::stringstream ss;
  ss << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(ptr);
  return ss.str();
}
