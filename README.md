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

## Running your own git-based fork

To allow for automatic building of the CGI program upon git push, you need
two git repositories on your webserver.  A bare repository which will also
contain the CGI program later on, and a working tree which will be used
to build the program.
If you configure the location of your working tree via
git config img2brl.src in the bare repository, the
[post-update hook](https://github.com/mlang/img2brl/blob/master/git-post-update-hook)
will automatically build and install changes to your repository upon push.

[img2brl@delysid](img2brl.delysid.org/) is approximately configured like this:

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
see if the program still compiles.  If you decide to commit and push your
changes, they will automatically get compiled on the remote webserver, and an
up-to-date binary (img2brl.cgi) will be installed into /home/user/img2brl.

    local$ git push
    Counting objects: 5, done.
    Delta compression using up to 4 threads.
    Compressing objects: 100% (3/3), done.
    Writing objects: 100% (3/3), 697 bytes | 0 bytes/s, done.
    Total 3 (delta 2), reused 0 (delta 0)
    remote: [100%] Built target img2brl.cgi
    remote: Install the project...
    remote: -- Install configuration: "RELEASE"
    remote: -- Up-to-date: /home/user/img2brl/img2brl.cgi
    remote: -- Up-to-date: /home/user/img2brl/favicon.png
    remote: -- Up-to-date: /home/user/img2brl/hooks/post-update
    To user@remote:/home/user/img2brl
       968e626..533f058  master -> master

All that is left now is to configure your web server to serve /home/user/img2brl/
(the bare repository), which is now a mixture of a git bare repository and the
CGI program img2brl.cgi.  You will likely need to enable CGI execution in
that directory, and perhaps also have img2brl.cgi served as the index page.

