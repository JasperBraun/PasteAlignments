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

#include "paste_alignments.h"

namespace Catch {

template<>
struct StringMaker<paste_alignments::Alignment> {
  static std::string convert(const paste_alignments::Alignment& a) {
    return a.DebugString();
  }
};

template<>
struct StringMaker<paste_alignments::ScoringSystem> {
  static std::string convert(const paste_alignments::ScoringSystem& s) {
    return s.DebugString();
  }
};

template<>
struct StringMaker<paste_alignments::AlignmentBatch> {
  static std::string convert(const paste_alignments::AlignmentBatch& batch) {
    return batch.DebugString();
  }
};

template<>
struct StringMaker<paste_alignments::AlignmentReader> {
  static std::string convert(const paste_alignments::AlignmentReader& reader) {
    return reader.DebugString();
  }
};

template<>
struct StringMaker<paste_alignments::PasteParameters> {
  static std::string convert(const paste_alignments::PasteParameters& p) {
    return p.DebugString();
  }
};

template<>
struct StringMaker<paste_alignments::StatsCollector> {
  static std::string convert(const paste_alignments::StatsCollector& s) {
    return s.DebugString();
  }
};

} // namespace Catch

#endif // PASTE_ALIGNMENTS_TEST_STRING_CONVERSIONS_H_