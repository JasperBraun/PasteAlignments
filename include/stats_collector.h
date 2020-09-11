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

namespace paste_alignments {

struct PasteStats {

  /// @brief Query sequence identifier.
  ///
  std::string qseqid;

  /// @brief Subject sequence identifier.
  ///
  std::string sseqid;

  /// @brief Number of alignments.
  ///
  long num_alignments;

  /// @brief Number of times alignments were pasted.
  ///
  long num_pastings;

  /// @brief Average alignment length.
  ///
  float average_length;

  /// @brief Average alignment percent identity.
  ///
  float average_pident;

  /// @brief Average alignment score.
  ///
  float average_score;

  /// @brief Average alignment bitscore.
  ///
  float average_bitscore;

  /// @brief Average alignment evalue.
  ///
  double average_evalue;

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
};

class StatsCollector {
 public:
  /// @name Factories:
  ///
  /// @{
  
  /// @brief Constructs object associating to it file named `file_name`.
  ///
  /// @parameter file_name The name of the file the object is associated with.
  ///
  /// @exceptions Basic guarantee. Attempts to create/open file named
  ///  `file_name`. Throws `exceptions::InvalidInput` if unable to open or
  ///  create file.
  ///
  inline static StatsCollector FromFile(std::string file_name) {
    StatsCollector result;

    result.ofs_.open(file_name, std::ofstream::trunc);
    if (result.ofs_.fail()) {
      std::stringstream error_message;
      error_message << "Unable to open file: '" << file_name << "'.";
      throw exceptions::InvalidInput(error_message.str());
    }

    return result;
  }
  /// @}
  
  /// @name Constructors:
  ///
  /// @{

  StatsCollector(const StatsCollector& other) = delete;

  /// @brief Move constructor.
  ///
  StatsCollector(StatsCollector&& other) = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{

  StatsCollector& operator=(const StatsCollector& other) = delete;

  /// @brief Move assignment.
  ///
  StatsCollector& operator=(StatsCollector&& other) = default;
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
  /// @details All averages and counts in return value are set to 0 if no stats
  ///  were computed.
  ///
  PasteStats WriteData();
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
  std::unique_ptr<std::ofstream> ofs_;
  std::vector<PasteStats> batch_stats_;
};

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_STATS_COLLECTOR_H_