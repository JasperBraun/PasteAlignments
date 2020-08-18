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

#ifndef PASTE_ALIGNMENTS_TEST_STRING_CONVERSIONS_H_
#define PASTE_ALIGNMENTS_TEST_STRING_CONVERSIONS_H_

#include "catch.h"

#include <sstream>

#include "alignment.h"

namespace Catch {

template<>
struct StringMaker<paste_alignments::Alignment> {
  static std::string convert(const paste_alignments::Alignment& alignment) {
    std::stringstream  ss;
    ss << std::boolalpha << "(id=" << alignment.id()
       << ", qstart=" << alignment.qstart()
       << ", qend=" << alignment.qend()
       << ", sstart=" << alignment.sstart()
       << ", send=" << alignment.send()
       << ", plus_strand=" << alignment.plus_strand()
       << ", nident=" << alignment.nident()
       << ", mismatch=" << alignment.mismatch()
       << ", gapopen=" << alignment.gapopen()
       << ", gaps=" << alignment.gaps()
       << ", qlen=" << alignment.qlen()
       << ", slen=" << alignment.slen()
       << ", qseq='" << alignment.qseq()
       << "', sseq='" << alignment.sseq()
       << "')";
    return ss.str();
  }
};

} // namespace Catch

#endif // PASTE_ALIGNMENTS_TEST_STRING_CONVERSIONS_H_