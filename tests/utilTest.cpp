#include <vector>
#include <string>
#include <gtest/gtest.h>

#include "utils/util.hpp"

class test_obj {
private:
  int a;
public:
  test_obj(int a): a(a) {}
  int getInt() const {return a;}
};

TEST(UtilTest, Collate) {
  std::vector<test_obj> vec1 {
    test_obj(1),
    test_obj(54),
    test_obj(72364),
    test_obj(985123)
  };
  auto collated1 = collate<decltype(vec1)>(vec1, [](test_obj x) {
    return std::to_string(x.getInt());
  });
  EXPECT_EQ(collated1, "1, 54, 72364, 985123");
  
  std::vector<std::string> vec2 {
    "asd",
    "lkjg",
    "234.234.24sdfplk"
  };
  auto collated2 = collate<decltype(vec2)>(vec2, [](std::string x) {
    return x;
  });
  EXPECT_EQ(collated2, "asd, lkjg, 234.234.24sdfplk");
  
  auto collated3 = collate<decltype(vec1)>(vec1,
  [](test_obj x) {
    return std::to_string(x.getInt());
  },
  [](std::string a, std::string b) {
    return a + " AYY LMAO " + b;
  });
  EXPECT_EQ(collated3, "1 AYY LMAO 54 AYY LMAO 72364 AYY LMAO 985123");
}

TEST(UtilTest, Split) {
  auto splitted = split("Lorem ipsum dolor sit amet", ' ');
  ASSERT_EQ(splitted, std::vector<std::string>({"Lorem", "ipsum", "dolor", "sit", "amet"}));
  auto empty = split("", ' ');
  ASSERT_EQ(empty, std::vector<std::string> {});
}
