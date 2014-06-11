/* Screen.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SCREEN_H_
#define SCREEN_H_

#include "Point.h"



// Class that simply holds the screen dimensions. This is really just a wrapper
// around some global variables, which means that no one but the drawing thread
// is allowed to use it.
class Screen {
public:
	static void Set(int width, int height);
	
	static int Width();
	static int Height();
	
	// Get the positions of the edges and corners of the viewport.
	static int Left();
	static int Top();
	static int Right();
	static int Bottom();
	
	static Point TopLeft();
	static Point TopRight();
	static Point BottomLeft();
	static Point BottomRight();
	
	
private:
	static int width;
	static int height;
};



#endif
