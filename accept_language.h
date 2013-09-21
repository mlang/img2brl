#include <vector>
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
};
