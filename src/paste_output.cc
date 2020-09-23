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

#include "paste_output.h"

namespace paste_alignments {

// WriteBatch
//
void WriteBatch(AlignmentBatch batch, std::ostream& os) {
  if (batch.Size() == 0) {return;}
  for (const Alignment& a : batch.Alignments()) {
    if (a.IncludeInOutput()) {
      os << batch.Qseqid()
         << '\t' << batch.Sseqid()
         << '\t' << a.Qstart()
         << '\t' << a.Qend();
      if (a.PlusStrand()) {
        os << '\t' << a.Sstart()
           << '\t' << a.Send();
      } else {
        os << '\t' << a.Send()
           << '\t' << a.Sstart();
      }
      os << '\t' << a.Nident()
         << '\t' << a.Mismatch()
         << '\t' << a.Gapopen()
         << '\t' << a.Gaps()
         << '\t' << a.Qlen()
         << '\t' << a.Slen()
         << '\t' << a.Length()
         << '\t' << a.Qseq()
         << '\t' << a.Sseq()
         << '\t' << a.Pident()
         << '\t' << a.RawScore()
         << '\t' << a.Bitscore()
         << '\t' << a.Evalue()
         << '\t' << a.Nmatches()
         << '\t' << a.PastedIdentifiers().at(0);
      for (int i = 1; i < static_cast<int>(a.PastedIdentifiers().size()); ++i) {
        os << ',' << a.PastedIdentifiers().at(i);
      }
      os << '\n';
    }
  }
}

} // namespace paste_alignments