// Copyright (c) 2020 Jasper Braun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ARG_PARSE_CONVERT_CONVERSION_FUNCTIONS_H_
#define ARG_PARSE_CONVERT_CONVERSION_FUNCTIONS_H_

#include <functional>
#include <string>

namespace arg_parse_convert {

namespace converters {

// Placeholder that is only used for flags. Never actually evaluated.
inline bool FlagConverter(const std::string& s) {return true;}

inline std::string StringIdentity(const std::string& s) {return s;}

inline int stoi(const std::string& s) {return std::stoi(s);}
inline long stol(const std::string& s) {return std::stol(s);}
inline long long stoll(const std::string& s) {return std::stoll(s);}
inline unsigned long stoul(const std::string& s) {return std::stoul(s);}
inline unsigned long long stoull(const std::string& s) {return std::stoull(s);}
inline float stof(const std::string& s) {return std::stof(s);}
inline double stod(const std::string& s) {return std::stod(s);}
inline long double stold(const std::string& s) {return std::stold(s);}

} // namespace stl_converters

} // namespace arg_parse_convert

#endif // ARG_PARSE_CONVERT_CONVERSION_FUNCTIONS_H_