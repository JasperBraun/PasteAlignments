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

#include "alignment_writer.h"

namespace paste_alignments {

// AlignmentWriter::WriteBatch
//
void WriteBatch(AlignmentBatch batch, const PasteParameters& parameters) {
  std::stringstream ss;
  if (line_break_) {
    ss << '\n';
  } else {
    line_break_ = true;
  }
  for (const Alignment& a : batch.Alignments()) {
    if (a.SatisfiesThresholds(parameters.final_pident_threshold,
                              parameters.final_score_threshold,
                              parameters)) {
      ss << batch.Qseqid()
         << '\t' << batch.Sseqid()
         << '\t' << a.Qstart()
         << '\t' << a.Qend();
      if (a.PlusStrand()) {
        ss << '\t' << a.Sstart()
           << '\t' << a.Send();
      } else {
        ss << '\t' << a.Send()
           << '\t' << a.Sstart();
      }
      ss << '\t' << a.Nident()
         << '\t' << a.Mismatch()
         << '\t' << a.Gapopen()
         << '\t' << a.Gaps()
         << '\t' << a.Qlen()
         << '\t' << a.Slen()
         << '\t' << a.Qseq()
         << '\t' << a.Sseq()
         << '\t' << a.Pident()
         << '\t' << a.RawScore()
         << '\t' << a.Bitscore()
         << '\t' << a.Evalue()
         << '\t' << a.PastedIdentifiers().at(0);
      for (int i = 1; i < a.PastedIdentifiers().size(); ++i) {
        ss << ',' << a.PastedIdentifiers().at(i);
      }
    }
  }
  if (ss.str().size() > 1) {
    ofs_.write(ss.str().data(), ss.str().size());
  }
}

// AlignmentWriter::DebugString
//
std::string AlignmentWriter::DebugString() const {
  std::stringstream ss;
  ss << "{ofs.is_open: " << std::boolalpha << ofs_.is_open() << '}';
  return ss.str();
}

} // namespace paste_alignments