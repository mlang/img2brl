#include <Magick++/Image.h>

class ubrl
{
  std::size_t w, h;
  std::string data;
public:
  ubrl(Magick::Image const &);
  std::size_t width() const { return w; }
  std::size_t height() const { return h; }
  std::string const &string() const { return data; }
};
