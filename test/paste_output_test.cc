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

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <set>
#include <sstream>
#include <vector>

#include "alignment.h"
#include "alignment_batch.h"
#include "exceptions.h"

// AlignmentWriter tests
//
// Test correctness for:
// * WriteBatch

namespace paste_alignments {

namespace test {

void AddRow(const std::string& qseqid, const std::string& sseqid,
            const Alignment& a, std::stringstream& ss) {
  ss << qseqid << '\t' << sseqid << '\t' << a.Qstart() << '\t' << a.Qend() << '\t';
  if (a.PlusStrand()) {
    ss << a.Sstart() << '\t' << a.Send();
  } else {
    ss << a.Send() << '\t' << a.Sstart();
  }
  ss << '\t' << a.Nident() << '\t' << a.Mismatch() << '\t' << a.Gapopen()
     << '\t' << a.Gaps() << '\t' << a.Qlen() << '\t' << a.Slen()
     << '\t' << a.Qseq() << '\t' << a.Sseq() << '\t' << a.Pident()
     << '\t' << a.RawScore() << '\t' << a.Bitscore()
     << '\t' << a.Evalue() << '\t' << a.PastedIdentifiers().at(0);
  for (int i = 1; i < a.PastedIdentifiers().size(); ++i) {
    ss << ',' << a.PastedIdentifiers().at(i);
  }
}

namespace {

SCENARIO("Test correctness of WriteBatch.", "[WriteBatch][correctness]") {

  GIVEN("A valid output stream and alignment data.") {
    std::stringstream ss, expected_ss;
    PasteParameters paste_parameters;
    ScoringSystem scoring_system{ScoringSystem::Create(400000l, 1, 2, 0, 0)};

    std::vector<AlignmentBatch> batches{
        AlignmentBatch{"qseq1", "sseq1"},
        AlignmentBatch{"qseq1", "sseq2"},
        AlignmentBatch{"qseq2", "sseq2"},
        AlignmentBatch{"sseq2", "qseq2"},
        AlignmentBatch{"qseq1", "sseq1"},
        AlignmentBatch{"qseq4", "sseq4"}};

    // Alignments are the same for each batch.
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                     "24", "1", "0", "0",
                                     "10000", "100000",
                                     "GCCCCAAAATTCCCCAAAATTCCCC",
                                     "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                     "20", "0", "0", "0",
                                     "10000", "100000",
                                     "CCCCAAAATTCCCCAAAATT",
                                     "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"101", "150", "1050", "1001",
                                     "40", "10", "0", "0",
                                     "10000", "100000",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                     "10", "0", "0", "0",
                                     "10000", "100000",
                                     "CCCCAAAATT",
                                     "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"101", "125", "1135", "1111",
                                     "20", "5", "0", "0",
                                     "10000", "100000",
                                     "GGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                     "30", "10", "0", "0",
                                     "10000", "100000",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                     "10", "5", "0", "0",
                                     "10000", "100000",
                                     "GGGGGCCCCAAAATT",
                                     "AAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                     "20", "15", "0", "0",
                                     "10000", "100000",
                                     "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                     "10", "10", "0", "0",
                                     "10000", "100000",
                                     "GGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                     "10", "15", "0", "0",
                                     "10000", "100000",
                                     "GGGGGGGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters)};

    WHEN("All alignments are marked as final.") {
      for (Alignment& a : alignments) {
        a.IncludeInOutput(true);
      }
      for (AlignmentBatch& batch : batches) {
        batch.ResetAlignments(alignments, paste_parameters);
      }
      THEN("All alignments are printed.") {
        for (AlignmentBatch& batch : batches) {
          for (const Alignment& a : alignments) {
            AddRow(batch.Qseqid(), batch.Sseqid(), a, expected_ss);
            expected_ss << '\n';
          }
          WriteBatch(std::move(batch), ss);
        }
        std::string expected{expected_ss.str()}, computed{ss.str()};
        CHECK(expected == computed);
      }
    }

    WHEN("No alignment is marked as final.") {
      for (Alignment& a : alignments) {
        a.IncludeInOutput(false);
      }
      for (AlignmentBatch& batch : batches) {
        batch.ResetAlignments(alignments, paste_parameters);
      }
      THEN("No alignments are printed.") {
        for (AlignmentBatch& batch : batches) {
          WriteBatch(std::move(batch), ss);
        }
        std::string expected{expected_ss.str()}, computed{ss.str()};
        CHECK(expected == computed);
      }
    }

    WHEN("Batches are empty.") {
      THEN("No alignments are printed.") {
        for (AlignmentBatch& batch : batches) {
          WriteBatch(std::move(batch), ss);
        }
        std::string expected{expected_ss.str()}, computed{ss.str()};
        CHECK(expected == computed);
      }
    }

    WHEN("Some alignments are pasted.") {
      for (AlignmentBatch& batch : batches) {
        batch.ResetAlignments(alignments, paste_parameters);
        batch.PasteAlignments(scoring_system, paste_parameters);
      }
      THEN("Pasted identifiers are listed in output.") {
        for (AlignmentBatch& batch : batches) {
          for (const Alignment& a : batch.Alignments()) {
            if (a.IncludeInOutput()) {
              AddRow(batch.Qseqid(), batch.Sseqid(), a, expected_ss);
              expected_ss << '\n';
            }
          }
          WriteBatch(std::move(batch), ss);
        }
        std::string expected{expected_ss.str()}, computed{ss.str()};
        CHECK(expected == computed);
      }
    }

    WHEN("Some but not all alignments are marked as final.") {
      std::vector<int> pos_of_final = GENERATE(take(3, chunk(6, random(1, 9))));
      std::set<int> unique_pos_of_final{pos_of_final.begin(),
                                        pos_of_final.end()};
      for (int pos : unique_pos_of_final) {
        alignments.at(pos).IncludeInOutput(true);
      }
      for (AlignmentBatch& batch : batches) {
        batch.ResetAlignments(alignments, paste_parameters);
      }
      THEN("Only marked alignments are printed.") {
        for (AlignmentBatch& batch : batches) {
          for (int pos : unique_pos_of_final) {
            AddRow(batch.Qseqid(), batch.Sseqid(), alignments.at(pos),
                   expected_ss);
            expected_ss << '\n';
          }
          WriteBatch(std::move(batch), ss);
        }
        std::string expected{expected_ss.str()}, computed{ss.str()};
        CHECK(expected == computed);
      }
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments