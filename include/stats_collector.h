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

#ifndef PASTE_ALIGNMENTS_STATS_COLLECTOR_H_
#define PASTE_ALIGNMENTS_STATS_COLLECTOR_H_

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "alignment_batch.h"
#include "exceptions.h"

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Collects several descriptive statistics related to alignment pasting.
///
struct PasteStats {

  /// @brief Query sequence identifier.
  ///
  std::string qseqid{""};

  /// @brief Subject sequence identifier.
  ///
  std::string sseqid{""};

  /// @brief Number of alignments.
  ///
  long num_alignments{0l};

  /// @brief Number of times alignments were pasted.
  ///
  long num_pastings{0l};

  /// @brief Average alignment length.
  ///
  float average_length{0.0f};

  /// @brief Average alignment percent identity.
  ///
  float average_pident{0.0f};

  /// @brief Average alignment score.
  ///
  float average_score{0.0f};

  /// @brief Average alignment bitscore.
  ///
  float average_bitscore{0.0f};

  /// @brief Average alignment evalue.
  ///
  double average_evalue{0.0};

  /// @brief Average number of aligned unknown residues counted as mismatches.
  ///
  float average_nmatches{0.0f};

  /// @name Other:
  ///
  /// @{

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
};

class StatsCollector {
 public:

  /// @name Constructors:
  ///
  /// @{

  /// @brief Default constructor.
  ///
  StatsCollector() = default;

  /// @brief Copy constructor.
  ///
  StatsCollector(const StatsCollector& other) = default;

  /// @brief Move constructor.
  ///
  StatsCollector(StatsCollector&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{

  /// @brief Copy assignment.
  ///
  StatsCollector& operator=(const StatsCollector& other) = default;

  /// @brief Move assignment.
  ///
  StatsCollector& operator=(StatsCollector&& other) = default;
  /// @}

  /// @name Accessors:
  ///
  /// @{
  
  /// @brief Returns all stored `BatchStats` objects.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline const std::vector<PasteStats>& BatchStats() const {
    return batch_stats_;
  }
  /// @}

  /// @name Stats computation:
  ///
  /// @{
  
  /// @brief Computes descriptive statistics for the batch of alignments.
  ///
  /// @parameter batch The batch for which statistics are computed.
  ///
  /// @details Only stores the batch's stats if it's not empty.
  ///
  /// @exceptions Strong guarantee.
  ///
  void CollectStats(const AlignmentBatch& batch);
  /// @}
  
  /// @name Write operations:
  ///
  /// @{
  
  /// @brief Writes all computed statistics into associated file and returns
  ///  overall statistics.
  ///
  /// @parameter os Stream to write statistics into.
  ///
  /// @details All averages and counts in return value are set to 0 if no stats
  ///  were computed.
  ///
  PasteStats WriteData(std::ostream& os);
  /// @}

  /// @name Other:
  ///
  /// @{
  
  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}
 private:
  std::vector<PasteStats> batch_stats_;
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_STATS_COLLECTOR_H_