template<typename T>
void print(T thing) {
  std::cout << thing;
}

template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(args...);
}

template <typename T>
bool contains(T element, std::vector<T> vec) {
  return std::find(vec.begin(), vec.end(), element) != vec.end();
}

template<typename T, typename Lambda>
std::vector<std::vector<T> > splitVector(std::vector<T> origin, Lambda& shouldSplit) {
  std::vector<std::vector<T> > vec2d {std::vector<T>()};
  long long vec2dIndex = 0;
  for (T elem : origin) {
    vec2d[vec2dIndex].push_back(elem);
    if (shouldSplit(elem)) {
      vec2dIndex++;
      vec2d.push_back(std::vector<T>());
    }
  }
  return vec2d;
}

template<typename T>
std::size_t hash(T element) {
  return std::hash<T>()(element);
}

template<typename T>
std::size_t hash(std::vector<T>& element) {
  if (element.size() == 0) return 0;
  std::size_t h = 0;
  for (auto e : element) h = hash(h, e);
  return h;
}

template<typename T, typename... HashTypes>
std::size_t hash(std::size_t init, T element, HashTypes... ht) {
  return hash(init ^ (hash<T>(element) + 0x9e3779b9 + (init << 6) + (init >> 2)), ht...);
}

template<typename T, typename... HashTypes>
std::size_t hash(T element, HashTypes... ht) {
  return hash(hash<T>(element), ht...);
}

// For some reason, this needs to be a duplicate of above hash(vector<T> element) function with different name
template<typename T>
std::size_t hashV(std::vector<T>& element) {
  if (element.size() == 0) return 0;
  std::size_t h = 0;
  for (auto e : element) h = hash(h, e);
  return h;
}

namespace std {
  template<class E>
  class hash<std::vector<E>> {
  public:
    size_t operator()(const vector<E>& element) const {
      return hashV(const_cast<vector<E>&>(element));
    }
  };
  template<class E>
  class hash {
    using sfinae = typename std::enable_if<std::is_enum<E>::value, E>::type;
  public:
    size_t operator()(const E&e) const {
      return std::hash<typename std::underlying_type<E>::type>()(e);
    }
  };
};
