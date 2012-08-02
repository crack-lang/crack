/*===========================================================================*\
     
    This header mainly defines the macro SPUG_FMT() - which can be used to
    format a spug::String in-place using the stream operator:

      void foo(const spug::String &str);
      foo(SPUG_FSTR("this is an integer: " << 100 << " this is a float: " <<
                    1.0
                    )
          )

    Copyright 2007 Michael A. Muller <mmuller@enduden.com>
    Copyright 2010 Google Inc.
    
      This Source Code Form is subject to the terms of the Mozilla Public
      License, v. 2.0. If a copy of the MPL was not distributed with this
      file, You can obtain one at http://mozilla.org/MPL/2.0/.
    
 
    This file is part of spug++.
 
    spug++ is free software: you can redistribute it and/or modify it under the 
    terms of the GNU Lesser General Public License as published by the Free 
    Software Foundation, either version 3 of the License, or (at your option) 
    any later version.
 
    spug++ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.
 
    You should have received a copy of the GNU Lesser General Public License 
    along with spug++.  If not, see <http://www.gnu.org/licenses/>.
 
\*===========================================================================*/

#ifndef SPUG_STRINGFMT_H
#define SPUG_STRINGFMT_H

#include <sstream>

namespace spug {
    /**
     * For some reason, on gcc 4.1, an std::ostringstream temporary doesn't 
     * behave the same as an ostring with regards to the "<<" operator - in 
     * particular, 'std::ostringstream() << "string val"' will treat the 
     * string as a void*, printing its address. This class solves that 
     * problem.
     */
    class StringFmt {
        private:
            std::ostringstream out;

        public:
            template <typename T>
            std::ostream &operator <<(const T &val) {
                out << val;
                return out;
            }
    };
}

#define SPUG_FSTR(msg) \
    static_cast<const std::ostringstream&>(spug::StringFmt() << msg).str()

#endif
