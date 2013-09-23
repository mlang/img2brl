#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm/stable_sort.hpp>
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
    boost::stable_sort(entries, &entry::compare_by_q);
  }
};

inline std::ostream &operator<<(std::ostream &stream, accept_language const &spec) {
  for (auto i = spec.languages().begin(), e = spec.languages().end(); i != e; ++i) {
    if (i != spec.languages().begin()) stream << ", ";
    for (auto j = i->subtags.begin(); j != i->subtags.end(); ++j) {
      if (j != i->subtags.begin()) stream << '-';
      stream << *j;
    }
    if (i->q) stream << "; q=" << *(i->q);
  }
  return stream;
}

