# A web service for converting arbitrary image data to braille representation

img2brl is a web service to convert arbitrary image formats to a tactile
representation based on unicode braille.  It can be used by a braille user
to gather information about an image file without needing sighted assistance.
A refreshable braille display or braille embosser will be required to make
use of the output of this program.

The idea is as simple as it can get: Provide an image file from your computer,
or enter an URL to an image on the web.  If the image can be read by
[ImageMagick](http://imagemagick.org/), it will be converted to Unicode Braille
and can be touched in the browser.

A live installation of this program can be found [here](http://img2brl.delysid.org/).

