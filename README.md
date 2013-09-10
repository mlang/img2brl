# A web service for converting arbitrary image data to braille representation

img2brl is a web service to convert arbitrary image formats to a tactile
representation based on
[Unicode braille](http://en.wikipedia.org/wiki/Unicode_braille).  It can be
used by a braille user to gather information about an image file without needing
sighted assistance.

A refreshable braille display or braille embosser will be needed to make
use of the output of this program.

The idea is as simple as it can get: Provide an image file from your computer,
or enter an URL to an image on the web.  If the image can be read by
[ImageMagick](http://imagemagick.org/), it will be converted to Unicode Braille
and can be touched in the browser.

A live installation of this program can be found
[here](http://img2brl.delysid.org/).

# Installation

Since all the functionality provided is contained in a single compiled
executable, installation can be very simple.

## Dependencies

You need

* [Git](http://git-scm.com/)
* [CMake](http://cmake.org/)
* A C++ compiler like [GCC](http://gcc.gnu.org/) or
  [Clang](http://clang.llvm.org/)
* [GNU cgicc](http://www.gnu.org/software/cgicc/)
* [ImageMagick](http://imagemagick.org/)
* [curl](http://curl.haxx.se/)

## Clone

    $ git clone http://img2brl.delysid.org/ img2brl

## Compile

    $ cd img2brl
    $ cmake . && make

## Run

You can now copy img2brl.cgi into your cgi-bin directory, and you should be
ready to go.

## Running your own git-based fork

To allow for automatic building of the CGI program upon git push, you need
two git repositories on your webserver.  A bare repository which will also
contain the CGI program later on, and a working tree which will be used
to build the program.
If you configure the location of your working tree via
git config img2brl.src in the bare repository, the
[post-update hook](https://github.com/mlang/img2brl/blob/master/git-post-update-hook)
will automatically build and install changes to your repository upon push.

[img2brl@delysid](http://img2brl.delysid.org/) is approximately configured like
this:

    remote$ git clone --bare https://github.com/mlang/img2brl img2brl
    remote$ git clone img2brl img2brl.src
    remote$ cd img2brl.src
    remote$ cmake -DCMAKE_INSTALL_PREFIX=../img2brl -DCMAKE_BUILD_TYPE=RELEASE .
    remote$ make install
    remote$ git --git-dir=../img2brl config img2brl.src $(pwd)
    remote$ exit
    local$ git clone remote:/home/user/img2brl
    local$ cd img2brl
    local$ cmake .

You can now do changes locally, and test them by locally running make, to
see if the program still compiles.  If you decide to push your
changes, they will automatically get compiled on the remote webserver, and an
up-to-date binary (img2brl.cgi) will be installed into /home/user/img2brl.

    local$ git push
    Counting objects: 16, done.
    Delta compression using up to 4 threads.
    Compressing objects: 100% (12/12), done.
    Writing objects: 100% (12/12), 1.41 KiB | 0 bytes/s, done.
    Total 12 (delta 7), reused 0 (delta 0)
    remote: -- Configuring done
    remote: -- Generating done
    remote: -- Build files have been written to: /home/user/img2brl.src
    remote: Scanning dependencies of target img2brl.cgi
    remote: [100%] Building CXX object CMakeFiles/img2brl.cgi.dir/img2brl.cc.o
    remote: Linking CXX executable img2brl.cgi
    remote: [100%] Built target img2brl.cgi
    remote: Install the project...
    remote: -- Install configuration: "RELEASE"
    remote: -- Installing: /home/user/img2brl/img2brl.cgi
    remote: -- Up-to-date: /home/user/img2brl/favicon.png
    remote: -- Up-to-date: /home/user/img2brl/hooks/post-update
    To user@remote:/home/user/img2brl
       1696b99..66316db  master -> master

All that is left now is to configure your remote webserver to serve
/home/user/img2brl at the URL of your choice.
You will likely need to enable CGI execution in
that directory, and perhaps also have img2brl.cgi served as the index page.

## Local testing

If you want to minimize mistakes on your online site, it can be helpful to
configure your local webserver to also serve an installation of img2brl.cgi.
You do not need to setup a bare repository, since your remote webserver
will already provide one.  You will likely want to provide CMAKE_INSTALL_PREFIX
to cmake in your local work tree and have it point to a location
which is served by your local webserver.  You can then very easily test
changes by executing "make install" in your local work tree, and visiting your
local webserver to observe the effect.

    local$ cmake -DCMAKE_INSTALL_PREFIX=~/public_html/img2brl .
    ...
    local$ make install
    local$ lynx http://localhost/~user

If you are happy with a change (or maybe after "git rebase -i") you can run
"git push" which will compile and install the current version on your remote
webserver.

