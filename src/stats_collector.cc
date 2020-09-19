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

namespace paste_alignments {

// PasteStats::DebugString
//
std::string PasteStats::DebugString() const {
  std::stringstream ss;
  ss << '('
     << "qseqid=" << qseqid
     << ", sseqid=" << sseqid
     << ", num_alignments=" << num_alignments
     << ", num_pastings=" << num_pastings
     << ", average_length=" << average_length
     << ", average_pident=" << average_pident
     << ", average_score=" << average_score
     << ", average_bitscore=" << average_bitscore
     << ", average_evalue=" << average_evalue
     << ", num_nmatches=" << num_nmatches
     << ')';
  return ss.str();
}

// StatsCollector::CollectStats
//
void StatsCollector::CollectStats(const AlignmentBatch& batch) {
  PasteStats stats;
  stats.qseqid = batch.Qseqid();
  stats.sseqid = batch.Sseqid();
  for (const Alignment& a : batch.Alignments()) {
    if (a.IncludeInOutput()) {
      stats.num_alignments += 1l;
      stats.num_pastings += static_cast<long>(a.PastedIdentifiers().size())
                            - 1l;
      stats.average_length += static_cast<float>(a.Length());
      stats.average_pident += a.Pident();
      stats.average_score += a.RawScore();
      stats.average_bitscore += a.Bitscore();
      stats.average_evalue += a.Evalue();
      stats.num_nmatches += static_cast<long>(a.Nmatches());
    }
  }
  if (stats.num_alignments > 0) {
    float f_num_alignments{static_cast<float>(stats.num_alignments)};
    stats.average_length /= f_num_alignments;
    stats.average_pident /= f_num_alignments;
    stats.average_score /= f_num_alignments;
    stats.average_bitscore /= f_num_alignments;
    stats.average_evalue /= static_cast<double>(stats.num_alignments);
    batch_stats_.emplace_back(std::move(stats));
  }
}

// StatsCollector::WriteData
//
PasteStats StatsCollector::WriteData(std::ostream& os) {
  PasteStats global_stats;
  if (!batch_stats_.empty()) {
    for (const PasteStats& s : batch_stats_) {
      global_stats.num_alignments += s.num_alignments;
      global_stats.num_pastings += s.num_pastings;
      global_stats.average_length += s.average_length
                                     * s.num_alignments;
      global_stats.average_pident += s.average_pident
                                     * s.num_alignments;
      global_stats.average_score += s.average_score
                                     * s.num_alignments;
      global_stats.average_bitscore += s.average_bitscore
                                     * s.num_alignments;
      global_stats.average_evalue += s.average_evalue
                                     * s.num_alignments;
      global_stats.num_nmatches += s.num_nmatches;

      os << s.qseqid
         << '\t' << s.sseqid
         << '\t' << s.num_alignments
         << '\t' << s.num_pastings
         << '\t' << s.average_length
         << '\t' << s.average_pident
         << '\t' << s.average_score
         << '\t' << s.average_bitscore
         << '\t' << s.average_evalue
         << '\t' << s.num_nmatches
         << '\n';
    }
    float f_num_alignments{static_cast<float>(global_stats.num_alignments)};
    global_stats.average_length /= f_num_alignments;
    global_stats.average_pident /= f_num_alignments;
    global_stats.average_score /= f_num_alignments;
    global_stats.average_bitscore /= f_num_alignments;
    global_stats.average_evalue /= static_cast<double>(f_num_alignments);
  }
  return global_stats;
}

// StatsCollector::DebugString
//
std::string StatsCollector::DebugString() const {
  std::stringstream ss;
  ss << '{'
     << "batch_stats: [";
  if (batch_stats_.size() > 0) {
    ss << batch_stats_.at(0).DebugString();
    for (int i = 1; i < batch_stats_.size(); ++i) {
      ss << ',' << batch_stats_.at(i).DebugString();
    }
  }
  ss << "]}";
  return ss.str();
}

} // namespace paste_alignments