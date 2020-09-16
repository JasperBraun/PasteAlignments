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

#ifndef PASTE_ALIGNMENTS_PASTE_OUTPUT_H_
#define PASTE_ALIGNMENTS_PASTE_OUTPUT_H_

#include <iostream>
#include <string>

#include "alignment_batch.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @name paste_output
///
/// @{

/// @brief Writes tab-separated alignment data from batch into data file.
///
/// @parameter batch The alignment batch to write.
/// @parameter os The stream to write the data into.
///
/// @details Column order: qseqid, sseqid, qstart, qend, sstart, send, nident,
///  gapopen, qlen, qseq, pident, score, bitscore, evalue, identifiers. Only
///  writes alignments which are marked as final.
///
void WriteBatch(AlignmentBatch batch, std::ostream& os = std::cout);
/// @}

/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_PASTE_OUTPUT_H_