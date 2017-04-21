#include <vector>
#include <string>
#include <gtest/gtest.h>

#include "utils/util.hpp"

class test_obj {
private:
  int a;
public:
  test_obj(int a): a(a) {}
  int getInt() {return a;}
  int getInt() const {return a;}
};

TEST(UtilTest, Collate) {
  std::vector<test_obj> vec1 {
    test_obj(1),
    test_obj(54),
    test_obj(72364),
    test_obj(985123)
  };
  auto collated1 = collate(vec1, [](test_obj x) {
    return std::to_string(x.getInt());
  });
  EXPECT_EQ(collated1, "1, 54, 72364, 985123");
  
  std::vector<std::string> vec2 {
    "asd",
    "lkjg",
    "234.234.24sdfplk"
  };
  std::string target2 = "asd, lkjg, 234.234.24sdfplk";
  auto collated2 = collate<decltype(vec2)>(vec2, [](std::string x) {
    return x;
  });
  EXPECT_EQ(collated2, target2);
  
  auto collated3 = collate<decltype(vec1)>(vec1,
  [](test_obj x) {
    return std::to_string(x.getInt());
  },
  [](std::string a, std::string b) {
    return a + " AYY LMAO " + b;
  });
  EXPECT_EQ(collated3, "1 AYY LMAO 54 AYY LMAO 72364 AYY LMAO 985123");
  
  auto collated4 = collate(vec2);
  EXPECT_EQ(collated4, target2);
}

TEST(UtilTest, Split) {
  auto splitted = split("Lorem ipsum dolor sit amet", ' ');
  ASSERT_EQ(splitted, std::vector<std::string>({"Lorem", "ipsum", "dolor", "sit", "amet"}));
  auto empty = split("", ' ');
  ASSERT_EQ(empty, std::vector<std::string> {});
}

TEST(UtilTest, ObjBind) {
  test_obj* obj = new test_obj(67);
  auto bound = objBind(&test_obj::getInt, obj);
  EXPECT_EQ(bound(), 67);
}
