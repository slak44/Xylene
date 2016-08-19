#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <unordered_map>
#include <string>

#include "object.hpp"
#include "utils/error.hpp"

class NotFoundError: public InternalError {
public:
  NotFoundError(ErrorData data):
    InternalError("NotFoundError", "Cannot find required identifier in this scope", data) {}
};

class Scope {
private:
  PtrUtil<Scope>::WeakLink parent;
  std::unordered_map<std::string, Reference::Link> map {};
public:
  using Link = PtrUtil<Scope>::Link;
  using WeakLink = PtrUtil<Scope>::WeakLink;
  
  Reference::WeakLink get(std::string ident) const {
    WeakLink lastParent = parent;
    while (!lastParent.expired()) {
      try {
        return map.at(ident);
      } catch (std::out_of_range& oor) {
        lastParent = parent.lock()->getParent();
      }
    }
    throw NotFoundError({METADATA_PAIRS, {"identifier", ident}});
  }
  
  void set(std::string ident, Object::Link obj) {
    try {
      map.at(ident)->setValue(obj);
    } catch (std::out_of_range& oor) {
      throw NotFoundError({METADATA_PAIRS, {"identifier", ident}});
    }
  }
  
  void insert(std::string ident, Reference::Link ref) {
    map.insert({ident, ref});
  }
  
  WeakLink getParent() const {
    return parent;
  }
  
  void setParent(WeakLink newParent) {
    parent = newParent;
  }
};

#endif
