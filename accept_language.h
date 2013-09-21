#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
class accept_language
{
public:
  struct entry {
    std::vector<std::string> subtags;
    boost::optional<float> q;
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
};
