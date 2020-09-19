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

#include "stats_collector.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <set>
#include <vector>

#include "helpers.h"

// AlignmentWriter tests
//
// Test correctness for:
// * CollectStats
// * WriteData

namespace paste_alignments {

namespace test {

PasteStats CalculateStats(const std::string& qseqid, const std::string& sseqid,
                          const std::set<int>& pos_of_final,
                          const std::vector<Alignment>& alignments) {
  PasteStats stats;
  stats.qseqid = qseqid;
  stats.sseqid = sseqid;
  stats.num_alignments = static_cast<long>(pos_of_final.size());
  float f_num_alignments{static_cast<float>(pos_of_final.size())};
  for (int pos : pos_of_final) {
    stats.num_pastings
        += static_cast<long>(alignments.at(pos).PastedIdentifiers().size())
           - 1l;
    stats.average_length += static_cast<float>(
                                   alignments.at(pos).Length());
    stats.average_pident += alignments.at(pos).Pident();
    stats.average_score += alignments.at(pos).RawScore();
    stats.average_bitscore += alignments.at(pos).Bitscore();
    stats.average_evalue += alignments.at(pos).Evalue();
    stats.average_nmatches += static_cast<float>(alignments.at(pos).Nmatches());
  }
  stats.average_length /= f_num_alignments;
  stats.average_pident /= f_num_alignments;
  stats.average_score /= f_num_alignments;
  stats.average_bitscore /= f_num_alignments;
  stats.average_evalue /= f_num_alignments;
  stats.average_nmatches /= f_num_alignments;
  return stats;
}

bool FuzzyEquals(const PasteStats& first, const PasteStats& second) {
  return (first.qseqid == second.qseqid
          && first.sseqid == second.sseqid
          && first.num_alignments == second.num_alignments
          && first.num_pastings == second.num_pastings
          && helpers::FuzzyFloatEquals(first.average_length,
                                       second.average_length)
          && helpers::FuzzyFloatEquals(first.average_pident,
                                       second.average_pident)
          && helpers::FuzzyFloatEquals(first.average_score,
                                       second.average_score)
          && helpers::FuzzyFloatEquals(first.average_bitscore,
                                       second.average_bitscore)
          && helpers::FuzzyDoubleEquals(first.average_evalue,
                                        second.average_evalue)
          && helpers::FuzzyFloatEquals(first.average_nmatches,
                                       second.average_nmatches));
}

void Print(const PasteStats& stats, std::ostream& os,
           PasteStats& cumulative_stats) {
  cumulative_stats.num_alignments += stats.num_alignments;
  cumulative_stats.num_pastings += stats.num_pastings;
  cumulative_stats.average_length += stats.average_length
                                     * stats.num_alignments;
  cumulative_stats.average_pident += stats.average_pident
                                     * stats.num_alignments;
  cumulative_stats.average_score += stats.average_score
                                     * stats.num_alignments;
  cumulative_stats.average_bitscore += stats.average_bitscore
                                     * stats.num_alignments;
  cumulative_stats.average_evalue += stats.average_evalue
                                     * stats.num_alignments;
  cumulative_stats.average_nmatches += stats.average_nmatches
                                       * stats.num_alignments;
  os << stats.qseqid
     << '\t' << stats.sseqid
     << '\t' << stats.num_alignments
     << '\t' << stats.num_pastings
     << '\t' << stats.average_length
     << '\t' << stats.average_pident
     << '\t' << stats.average_score
     << '\t' << stats.average_bitscore
     << '\t' << stats.average_evalue
     << '\t' << stats.average_nmatches
     << '\n';
}

void Average(PasteStats& stats) {
  float f_num_alignments{static_cast<float>(stats.num_alignments)};
  stats.average_length /= f_num_alignments;
  stats.average_pident /= f_num_alignments;
  stats.average_score /= f_num_alignments;
  stats.average_bitscore /= f_num_alignments;
  stats.average_evalue /= static_cast<double>(f_num_alignments);
  stats.average_nmatches /= f_num_alignments;
}

namespace {

SCENARIO("Test correctness of StatsCollector::CollectStats.",
         "[StatsCollector][CollectStats][correctness]") {
  PasteParameters paste_parameters;
  AlignmentBatch alignment_batch{"qseqid", "sseqid"};
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 0, 0)};
  StatsCollector collector;

  GIVEN("Empty batch.") {

    THEN("No stats are collected.") {
      collector.CollectStats(alignment_batch);
      CHECK(collector.BatchStats().empty());
    }
  }

  GIVEN("Batch with alignments.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                     "24", "1", "0", "0",
                                     "10000", "100000", "25",
                                     "GCCCCAAAATTCCCCAAAATTCCCC",
                                     "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                     "20", "0", "0", "0",
                                     "10000", "100000", "20",
                                     "CCCCAAAATTCCCCAAAATT",
                                     "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"101", "150", "1050", "1001",
                                     "40", "10", "0", "0",
                                     "10000", "100000", "50",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                     "10", "0", "0", "0",
                                     "10000", "100000", "10",
                                     "CCCCAAAATT",
                                     "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"101", "125", "1135", "1111",
                                     "20", "5", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                     "30", "10", "0", "0",
                                     "10000", "100000", "40",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                     "10", "5", "0", "0",
                                     "10000", "100000", "15",
                                     "GGGGGCCCCAAAATT",
                                     "AAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                     "20", "15", "0", "0",
                                     "10000", "100000", "35",
                                     "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                     "10", "10", "0", "0",
                                     "10000", "100000", "20",
                                     "GGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                     "10", "15", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGGGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters)};

    WHEN("None are marked final.") {
      alignment_batch.ResetAlignments(alignments, paste_parameters);

      THEN("No stats are collected.") {
        collector.CollectStats(alignment_batch);
        CHECK(collector.BatchStats().empty());
      }
    }

    WHEN("At least one is marked final.") {
      std::vector<int> pos_of_final = GENERATE(take(3, chunk(6, random(0, 9))));
      std::set<int> unique_pos_of_final{pos_of_final.begin(),
                                        pos_of_final.end()};
      for (int pos : unique_pos_of_final) {
        alignments.at(pos).IncludeInOutput(true);
      }
      alignment_batch.ResetAlignments(alignments, paste_parameters);

      THEN("Stats are collected.") {
        collector.CollectStats(alignment_batch);
        PasteStats expected{CalculateStats(alignment_batch.Qseqid(),
                                           alignment_batch.Sseqid(),
                                           unique_pos_of_final, alignments)};
        CHECK(FuzzyEquals(collector.BatchStats().at(0), expected));
      }
    }
  }

  GIVEN("Batch with pastable alignments.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "110", "1001", "1010",
                                        "10", "0", "0", "0",
                                        "10000", "100000", "10",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"111", "130", "1011", "1030",
                                        "20", "0", "0", "0",
                                        "10000", "100000", "20",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"131", "145", "1031", "1045",
                                        "15", "0", "0", "0",
                                        "10000", "100000", "15",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"146", "160", "1046", "1060",
                                        "15", "0", "0", "0",
                                        "10000", "100000", "15",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"41", "50", "1120", "1111",
                                        "10", "0", "0", "0",
                                        "10000", "100000", "10",
                                        "AAAAAAAAAA",
                                        "AAAAAAAAAA"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"51", "70", "1110", "1091",
                                        "20", "0", "0", "0",
                                        "10000", "100000", "20",
                                        "CCCCCCCCCCCCCCCCCCCC",
                                        "CCCCCCCCCCCCCCCCCCCC"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"71", "85", "1090", "1076",
                                        "15", "0", "0", "0",
                                        "10000", "100000", "15",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"86", "100", "1075", "1061",
                                        "15", "0", "0", "0",
                                        "10000", "100000", "15",
                                        "GGGGGGGGGGGGGGG",
                                        "GGGGGGGGGGGGGGG"},
                                       scoring_system, paste_parameters)};
    alignment_batch.ResetAlignments(alignments, paste_parameters);
    alignment_batch.PasteAlignments(scoring_system, paste_parameters);

    THEN("Alignments marked final contribute to collected stats.") {
      collector.CollectStats(alignment_batch);
      std::set<int> pos_of_final{1, 5};
      PasteStats expected{CalculateStats(alignment_batch.Qseqid(),
                                         alignment_batch.Sseqid(),
                                         pos_of_final,
                                         alignment_batch.Alignments())};
      CHECK(FuzzyEquals(collector.BatchStats().at(0), expected));
    }
  }

  GIVEN("Multiple alignment batches.") {
    std::vector<Alignment> alignments1{
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                     "24", "1", "0", "0",
                                     "10000", "100000", "25",
                                     "GCCCCAAAATTCCCCAAAATTCCCC",
                                     "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                     "20", "0", "0", "0",
                                     "10000", "100000", "20",
                                     "CCCCAAAATTCCCCAAAATT",
                                     "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"101", "150", "1050", "1001",
                                     "40", "10", "0", "0",
                                     "10000", "100000", "50",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::set<int> pos_of_final1{0,2};
    for (int pos : pos_of_final1) {
      alignments1.at(pos).IncludeInOutput(true);
    }
    std::vector<Alignment> alignments2{
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                     "10", "0", "0", "0",
                                     "10000", "100000", "10",
                                     "CCCCAAAATT",
                                     "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"101", "125", "1135", "1111",
                                     "20", "5", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::vector<Alignment> alignments3{
        Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                     "30", "10", "0", "0",
                                     "10000", "100000", "40",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::set<int> pos_of_final3{0};
    for (int pos : pos_of_final3) {
      alignments3.at(pos).IncludeInOutput(true);
    }
    std::vector<Alignment> alignments4{};
    std::vector<Alignment> alignments5{
        Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                     "10", "5", "0", "0",
                                     "10000", "100000", "15",
                                     "GGGGGCCCCAAAATT",
                                     "AAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                     "20", "15", "0", "0",
                                     "10000", "100000", "35",
                                     "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                     "10", "10", "0", "0",
                                     "10000", "100000", "20",
                                     "GGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                     "10", "15", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGGGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::set<int> pos_of_final5{0,1,2,3};
    for (int pos : pos_of_final5) {
      alignments5.at(pos).IncludeInOutput(true);
    }
    AlignmentBatch batch1{"qseqid1", "sseqid1"}, batch2{"qseqid2", "sseqid2"},
                   batch3{"qseqid1", "sseqid1"}, batch4{"qseqid2", "sseqid2"},
                   batch5{"qseqid1", "sseqid1"};
    batch1.ResetAlignments(alignments1, paste_parameters);
    batch2.ResetAlignments(alignments2, paste_parameters);
    batch3.ResetAlignments(alignments3, paste_parameters);
    batch4.ResetAlignments(alignments4, paste_parameters);
    batch5.ResetAlignments(alignments5, paste_parameters);

    THEN("Stats for batches containing final alignments are stored.") {
      collector.CollectStats(batch1);
      collector.CollectStats(batch2);
      collector.CollectStats(batch3);
      collector.CollectStats(batch4);
      collector.CollectStats(batch5);
      CHECK(collector.BatchStats().size() == 3);
      PasteStats expected1{CalculateStats(batch1.Qseqid(),
                                          batch1.Sseqid(),
                                          pos_of_final1,
                                          batch1.Alignments())};
      PasteStats expected3{CalculateStats(batch3.Qseqid(),
                                          batch3.Sseqid(),
                                          pos_of_final3,
                                          batch3.Alignments())};
      PasteStats expected5{CalculateStats(batch5.Qseqid(),
                                          batch5.Sseqid(),
                                          pos_of_final5,
                                          batch5.Alignments())};
      CHECK(FuzzyEquals(collector.BatchStats().at(0), expected1));
      CHECK(FuzzyEquals(collector.BatchStats().at(1), expected3));
      CHECK(FuzzyEquals(collector.BatchStats().at(2), expected5));
    }
  }
}

SCENARIO("Test correctness of StatsCollector::WriteData.",
         "[StatsCollector][WriteData][correctness]") {
  PasteParameters paste_parameters;
  ScoringSystem scoring_system{ScoringSystem::Create(100000l, 1, 2, 0, 0)};
  std::stringstream computed_ss, expected_ss;
  PasteStats computed_return_value, expected_return_value;

  THEN("An empty collector will not print anything and return empty stats.") {
    StatsCollector collector;
    computed_return_value = collector.WriteData(computed_ss);
    CHECK(computed_ss.str().empty());
    CHECK(FuzzyEquals(computed_return_value, expected_return_value));
  }

  GIVEN("A collector containing data for a single batch.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                     "24", "1", "0", "0",
                                     "10000", "100000", "25",
                                     "GCCCCAAAATTCCCCAAAATTCCCC",
                                     "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                     "20", "0", "0", "0",
                                     "10000", "100000", "20",
                                     "CCCCAAAATTCCCCAAAATT",
                                     "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"101", "150", "1050", "1001",
                                     "40", "10", "0", "0",
                                     "10000", "100000", "50",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                     "10", "0", "0", "0",
                                     "10000", "100000", "10",
                                     "CCCCAAAATT",
                                     "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"101", "125", "1135", "1111",
                                     "20", "5", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                     "30", "10", "0", "0",
                                     "10000", "100000", "40",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                     "10", "5", "0", "0",
                                     "10000", "100000", "15",
                                     "GGGGGCCCCAAAATT",
                                     "AAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                     "20", "15", "0", "0",
                                     "10000", "100000", "35",
                                     "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                     "10", "10", "0", "0",
                                     "10000", "100000", "20",
                                     "GGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                     "10", "15", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGGGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::set<int> pos_of_final;
    for (int pos = 0; static_cast<int>(pos < alignments.size()); ++pos) {
      pos_of_final.insert(pos);
      alignments.at(pos).IncludeInOutput(true);
    }
    AlignmentBatch batch{"qseqid1", "qseqid2"};
    batch.ResetAlignments(alignments, paste_parameters);
    StatsCollector collector;
    collector.CollectStats(batch);

    THEN("Collector writes one row of data and returns overall summary.") {
      PasteStats stats = CalculateStats(batch.Qseqid(), batch.Sseqid(),
                                        pos_of_final, alignments);
      Print(stats, expected_ss, expected_return_value);
      Average(expected_return_value);
      computed_return_value = collector.WriteData(computed_ss);
      CHECK(FuzzyEquals(expected_return_value, computed_return_value));
      CHECK(expected_ss.str() == computed_ss.str());
    }
  }

  GIVEN("A collector containing data for a multiple batches.") {
    std::vector<Alignment> alignments{
        Alignment::FromStringFields(0, {"101", "125", "1101", "1125",
                                     "24", "1", "0", "0",
                                     "10000", "100000", "25",
                                     "GCCCCAAAATTCCCCAAAATTCCCC",
                                     "ACCCCAAAATTCCCCAAAATTCCCC"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(1, {"101", "120", "1131", "1150",
                                     "20", "0", "0", "0",
                                     "10000", "100000", "20",
                                     "CCCCAAAATTCCCCAAAATT",
                                     "CCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(2, {"101", "150", "1050", "1001",
                                     "40", "10", "0", "0",
                                     "10000", "100000", "50",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(3, {"101", "110", "2111", "2120",
                                     "10", "0", "0", "0",
                                     "10000", "100000", "10",
                                     "CCCCAAAATT",
                                     "CCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(4, {"101", "125", "1135", "1111",
                                     "20", "5", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(5, {"101", "140", "1121", "1160",
                                     "30", "10", "0", "0",
                                     "10000", "100000", "40",
                                     "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(6, {"101", "115", "1096", "1110",
                                     "10", "5", "0", "0",
                                     "10000", "100000", "15",
                                     "GGGGGCCCCAAAATT",
                                     "AAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(7, {"101", "135", "101", "135",
                                     "20", "15", "0", "0",
                                     "10000", "100000", "35",
                                     "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(8, {"101", "120", "201", "220",
                                     "10", "10", "0", "0",
                                     "10000", "100000", "20",
                                     "GGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters),
        Alignment::FromStringFields(9, {"101", "125", "2101", "2125",
                                     "10", "15", "0", "0",
                                     "10000", "100000", "25",
                                     "GGGGGGGGGGGGGGGCCCCAAAATT",
                                     "AAAAAAAAAAAAAAACCCCAAAATT"},
                                    scoring_system, paste_parameters)};
    std::set<int> pos_of_final;
    for (int pos = 0; static_cast<int>(pos < alignments.size()); ++pos) {
      pos_of_final.insert(pos);
      alignments.at(pos).IncludeInOutput(true);
    }
    int num_batches = GENERATE(take(5, random(2, 10)));
    StatsCollector collector;
    std::string qseqid, sseqid;
    PasteStats stats;
    for (int i = 1; i <= num_batches; ++i) {
      qseqid = "qseqid";
      sseqid = "sseqid";
      qseqid.append(std::to_string(i));
      sseqid.append(std::to_string(i));
      AlignmentBatch batch{qseqid, sseqid};
      batch.ResetAlignments(alignments, paste_parameters);
      stats = CalculateStats(batch.Qseqid(), batch.Sseqid(), pos_of_final,
                             alignments);
      Print(stats, expected_ss, expected_return_value);
      collector.CollectStats(batch);
    }

    THEN("Collector writes a data row per batch and returns overall summary.") {
      Average(expected_return_value);
      computed_return_value = collector.WriteData(computed_ss);
      CHECK(FuzzyEquals(expected_return_value, computed_return_value));
      CHECK(expected_ss.str() == computed_ss.str());
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments