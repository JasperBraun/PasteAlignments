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

#ifndef PASTE_ALIGNMENTS_PASTE_PARAMETERS_H_
#define PASTE_ALIGNMENTS_PASTE_PARAMETERS_H_

namespace paste_alignments {

/// @addtogroup PasteAlignments-Reference
///
/// @{

/// @brief Collects all parameter values relevant for the program.
///
struct PasteParameters {

  /// @name Pasting parameters:
  ///
  /// @{
  
  /// @brief Maximum gap size introduced via pasting.
  ///
  int gap_tolerance{10};

  /// @brief Minimum percent identity for intermediate pasted alignments.
  ///
  float intermediate_pident_threshold{0.0f};

  /// @brief Minimum score for intermediate pasted alignments.
  ///
  float intermediate_score_threshold{0.0f};

  /// @brief Minimum percent identity for returned pasted alignments.
  ///
  float final_pident_threshold{0.0f};

  /// @brief Minimum score for returned pasted alignments.
  ///
  float final_score_threshold{0.0f};

  /// @brief When executed in blind mode, nucleotide sequences are disregarded.
  ///
  bool blind_mode{false};
  /// @}

  /// @name Scoring parameters:
  ///
  /// @{
  
  /// @brief Match reward used to compute score, bitscore and evalue.
  ///
  /// @details Positive value. Added to the score for each identical pair of
  ///  of residues in an alignment.
  ///
  int reward{1};

  /// @brief Mismatch penalty used to compute score, bitscore and evalue.
  ///
  /// @details Positive value. Subtracted from the score for each pair of
  ///  non-identical residues in an alignment.
  ///
  int penalty{2};

  /// @brief Gap opening cost used to compute score, bitscore and evalue.
  ///
  /// @details Positive value. Subtracted from the score for each consecutive
  ///  run of gap characters (`-`) on either side of an alignment.
  ///
  int open_cost{0};

  /// @brief Gap extension cost used to compute score, bitscore and evalue.
  ///
  /// @details Positive value. Subtracted from the score for each pair
  ///  consisting of a residue and a gap character (`-`) in an alignment.
  ///
  int extend_cost{0};

  /// @brief Size of the database of the input data.
  ///
  /// @details Sum of the sizes of the subject sequences which were searched for
  ///  alignments.
  ///
  long db_size;
  /// @}

  /// @name Input/Output:
  ///
  /// @{
  
  /// @brief Input data file.
  ///
  std::string input_filename;

  /// @brief Output data file.
  ///
  std::string output_filename;

  /// @brief Summary file.
  ///
  std::string summary_filename;

  /// @brief Statistics data file.
  ///
  std::string stats_filename;
  /// @}
  
  /// @name Other:
  ///
  /// @{
  
  /// @brief Maximum size of the chunks of input data read.
  ///
  long batch_size{256l * 1000l * 1000l};

  /// @brief Parameter that determines how far apart two floating points may be
  ///  to be considered equal.
  ///
  /// @details Multiplies with the smaller of two magnitudes of two floats to
  ///  to determine maximum distance.
  ///
  float float_epsilon{0.01f};

  /// @brief Parameter that determines how far apart two floating points with
  ///  double precision may be to be considered equal.
  ///
  /// @details Multiplies with the smaller of two magnitudes of two doubles to
  ///  to determine maximum distance.
  ///
  double double_epsilon{0.01};
  /// @}

  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline std::string DebugString() const {
    std::stringstream ss;
    ss << '{'
       << "gap_tolerance=" << gap_tolerance
       << ", i_pident_t=" << intermediate_pident_threshold
       << ", i_score_t=" << intermediate_score_threshold
       << ", f_pident_t=" << final_pident_threshold
       << ", f_score_t=" << final_score_threshold
       << ", blind_mode=" << blind_mode
       << ", reward=" << reward
       << ", penalty=" << penalty
       << ", open_cost=" << open_cost
       << ", extend_cost=" << extend_cost
       << ", db_size=" << db_size
       << ", input_filename=" << input_filename
       << ", output_filename=" << output_filename
       << ", summary_filename=" << summary_filename
       << ", stats_filename=" << stats_filename
       << ", batch_size=" << batch_size
       << ", float_epsilon=" << float_epsilon
       << ", double_epsilon=" << double_epsilon
       << '}';
    return ss.str();
  }
};
/// @}

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_PASTE_PARAMETERS_H_