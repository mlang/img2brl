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

# Installation

Since all the functionality provided is contained in a single compiled
executable, installation can be very simple.  You need
[CMake](http://cmake.org/) and a C++ compiler like [GCC](http://gcc.gnu.org/)
or [Clang](http://clang.llvm.org/) to build the CGI program.

    $ git clone http://img2brl.delysid.org/ img2brl
    $ cd img2brl
    $ cmake . && make

You can now copy img2brl.cgi into your cgi-bin directory, and you should be
ready to go.

## Git bare repository

To provide the source code repository at the same location as the CGI program,
you can create a bare git repository at the location which is served by your
web server.  If you then check out this bare repository at some other location,
you can build and install the CGI program and related files into the git
bare repository.  If you configure the location of your checkout via
git config img2brl.src in the bare repository, you can use a post-update
hook to automatically build and install changes to your repository upon push.

[img2brl@delysid](img2brl.delysid.org/) is approximately configured like this:

    local$ ssh remote
    remote$ mkdir img2brl
    remote$ cd img2brl
    remote$ git init --bare
    remote$ exit
    local$ git remote add web remote:/home/user/img2brl
    local$ git push -u web master
    local$ ssh remote
    remote$ mkdir img2brl.src
    remote$ cd img2brl.src
    remote$ git clone ../img2brl
    remote$ cmake -DCMAKE_INSTALL_PREFIX=/home/user/img2brl -DCMAKE_BUILD_TYPE=RELEASE .
    remote$ make install
    remote$ cd ../img2brl
    remote$ git config img2brl.src /home/user/img2brl.src
    remote$ exit

You can now do changes locally, and test them by locally running make, to
see if the program still compiles.  If you decide to commit and push your
changes, they will automatically get compiled on the remote server, and an
up-to-date binary (img2brl.cgi) will be installed into /home/user/img2brl/.

