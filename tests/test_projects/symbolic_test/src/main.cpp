/*
    SymbolicC++ : An object oriented computer algebra system written in C++

    Copyright (C) 2007 Yorick Hardy and Willi-Hans Steeb

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <iostream>
#include "symbolicc++.h"
using namespace std;

int main(void)
{
 Symbolic alpha("alpha");
 Symbolic X, XI, dX, result;

 X = ( ( cos(alpha), sin(alpha) ),
       (-sin(alpha), cos(alpha) ) );

 cout << X << endl;

 XI = X[alpha == -alpha]; cout << XI << endl;
 dX = df(X, alpha);       cout << dX << endl;

 result = XI * dX;        cout << result << endl;
 result = result[(cos(alpha)^2) == 1 - (sin(alpha)^2)];

 Symbolic y("3*x + 1 + 5*x");

 cout << result << endl;
 cout << y.simplify() << endl;
 X = y.simplify();
 cout << X + y << endl;

 return 0;
}