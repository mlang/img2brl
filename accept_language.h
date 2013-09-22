#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
class accept_language
{
public:
  struct entry {
    std::vector<std::string> subtags;
    boost::optional<float> q;
    static bool compare_by_q(entry const &lhs, entry const &rhs) {
      return lhs.q? (rhs.q? (*lhs.q - *rhs.q) > 0.0001
                          : *lhs.q > 1.0001)
                  : rhs.q and (*rhs.q < 0.9999);
    }
  };
private:
  std::vector<entry> entries;
public:
  accept_language(std::string const &);
  std::vector<entry> const &languages() const { return entries; }
  bool accepts_language(std::string const &lang) const
  {
    for (entry const &language: entries) {
      if (not language.subtags.empty()) {
        if (boost::iequals(language.subtags.front(), lang)) return true;
      }
    }
    return false;
  }
  void normalize() {
    auto first_q = entries.end();
    for (auto i = entries.begin(); i != entries.end(); ++i) {
      if (i->q) {
        if (first_q == entries.end() and *(i->q) < 0.9999) first_q = i;
      } else if (first_q != entries.end()) {
        std::rotate(first_q++, i, std::next(i));
      }
    }
    if (first_q != entries.end())
      std::stable_sort(first_q, entries.end(), &entry::compare_by_q);
  }
};
