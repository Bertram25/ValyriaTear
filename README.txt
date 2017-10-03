 _   _       _            _         _____
| | | |     | |          (_)       |_   _|
| | | | __ _| |_   _ _ __ _  __ _    | | ___  __ _ _ __
| | | |/ _` | | | | | '__| |/ _` |   | |/ _ \/ _` | '__|
\ \_/ / (_| | | |_| | |  | | (_| |   | |  __/ (_| | |
 \___/ \__,_|_|\__, |_|  |_|\__,_|   \_/\___|\__,_|_|
                __/ |
               |___/


**Development blog:** https://valyriatear.blogspot.com
**Bug Tracker:** https://github.com/ValyriaTearValyriaTear/issues
**Forums:** https://forum.freegamedev.net/viewforum.php?f=76

Valyria Tear is an open-source single-player medieval-fantasy 2D J-RPG based on an extended Hero of Allacrost engine
and inspired by classic console RPGs. See: http://www.allacrost.org

The player incarnates Bronann, a young boy forced to take part into the struggle for the possession of a mysterious crystal.
This powerful artifact will lead him to discover the actual threat set upon his world, but also courage and love.

The game has all the features you can expect from JRPG classics: entertaining story development, colourful map exploration,
active side-view battles, character management, puzzles... It is also translated in several languages.

## Licensing ##

Copyright (C) 2012-2017 by Bertram
Copyright (C) 2004-2011 by The Allacrost Project

The source code is licensed under the *GNU GPL*. It is free software and you may
modify it and/or redistribute it under the terms of this license.
See https://www.gnu.org/copyleft/gpl.html for details.

The graphics, sounds, musics and script files are licensed according to the LICENSES file:
https://raw.githubusercontent.com/ValyriaTear/ValyriaTear/master/LICENSES

While the scripts are under the GNU GPL, the story concept is copyrighted
and cannot be reused as is or part of it in another project without the author's consent.
This means you can package, and distribute Valyria Tear under the term of this license
but you cannot take the story or parts of it in another project without agreement.

The game directories included and used are:
data/ po/

## Compilation procedure ##

**Linux compilation**
Once you've unpacked the tarball, if you have checked it out from the Git repository, run this first to get the latest luabind code:
`git submodule update --init --remote --force`
and then the usual `cmake . && make` from the top-level directory at the prompt.

You'll then be able to play by typing: `src/valyriatear`

**Windows compilation**
- **Code::Blocks:**

A [Code::Blocks](http://www.codeblocks.org/) project file is also provided when compiling under Windows.
In that case, you might need the dependencies and headers files that can be downloaded here:
https://sourceforge.net/projects/valyriatear/files/win32-depends/valyriatear-win32-depends-sdl1.2-2014-12-11.zip/download

- **Microsoft Visual C++ 2013 and later:**

_**(Warning: not officially supported!)**_
Please contact Authenticate for support about this one: https://github.com/authenticate
To build on Windows with Visual Studio 2013, do this:
1. git clone https://github.com/authenticate/ValyriaTear-VS2013-Dependencies.git
2. git submodule update --recursive --init --remote --force
3. Copy the ValyriaTear-VS2013-Dependencies folder into your Valyria Tear repository.
4. Open the VS 2013 solution file: .../ValyriaTearRepository/vs2013/ValyriaTear.sln
5. Build (F7)
6. Run (F5)

**Mac OS X compilation**
- Use Macports or Homebrew to install things like boost.
- Open Xcode project, to quickly update the files, delete the 'src' folder in project and add it again. Remove all unnecessary things.
- Press "Play"

## Dependencies ##

You will need the following in order to compile and run the game:

- **Boost headers** (Tested from 1.42 to 1.58)
- **GNU Gettext** - (libiconv, libintl)
- **lua** (5.1.x - 5.2.x - 5.3.x) - (liblua)
- **libz**
- **libpng** (1.2 to 1.6)
- **Ogg/Vorbis** - (libogg, libvorbis, libvorbisenc, libvorbisfile)
- **OpenAL**
- **OpenGL**, **GLU** and **GLEW**
- **SDL** (2.0.0+)
- **SDL_ttf** (2.0.12+)
- **SDL_image** (2.0.0)

## Debug feature support compilation ##

- **Using Cmake:**
Use the `-D` parameter when invoking cmake to add **compilation flags**.

- **Add debug menus, and debug commands:**
`cmake -DDEBUG_FEATURES=on .`

- On **Code::Blocks:**
Go to Project->Build options, and add the flags in the `#defines` tab, i.e.:
`DEBUG_MENU`
