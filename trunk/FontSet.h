/* FontSet.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef FONT_SET_H_
#define FONT_SET_H_

#include <string>

class Font;



// Class for storing all the fonts that can be used.
class FontSet {
public:
	static void Add(const std::string &path, int size);
	static const Font &Get(int size);
};



#endif
