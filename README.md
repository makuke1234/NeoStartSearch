# NeoStartSearch

[![Release version](https://img.shields.io/github/v/release/makuke1234/NeoStartSearch?display_name=release&include_prereleases)](https://github.com/makuke1234/NeoStartSearch/releases/latest)
[![Total downloads](https://img.shields.io/github/downloads/makuke1234/NeoStartSearch/total)](https://github.com/makuke1234/NeoStartSearch/releases)
![C version](https://img.shields.io/badge/version-C11-blue.svg)

A new search engine for Windows designed to search from the Start menu.

The program is written in pure C and uses some standard Win32 API functions in order to function. MinGW has been used as the compiler.
I do give credit to Microsoft for creating this awesome icon, originally located in imageres.dll, that my application uses.


# Get started

The x86 Windows binary can be downloaded [here](https://github.com/makuke1234/NeoStartSearch/raw/main/NSSearch.exe).


# Usage

Just start the program and use the following keybindings to interact with it.

Keybindings:
| Key                     | Action |
| ----------------------- | ------ |
| ESC                     | Closes the search window |
| Win+F                   | (re)opens the search window |
| Alt+F4                  | Closes the program |
| Up-arrow / wheel-up     | Move up in the list of results |
| Down-arrow / wheel-down | Move down in the list of results |


# Features (changelog)

* v0.3:
	- [x] Scroll the list with mouse
	- [x] Per-character highlighting to see which parts of the keyword match
* v0.2:
	- [x] Proper mouse navigation between results
	- [x] Optimized rendering while selecting items
	- [ ] Scroll the list with mouse
* v0.1:
	- [x] Search from the local and global start menu
	- [x] Display icons alongside search results, although some icons aren't visible
	- [x] Dynamically expanding search box with a limit of MAX_PATH (260 characters)
	- [ ] Proper mouse navigation between results


# License

As stated above, the project uses the MIT license.