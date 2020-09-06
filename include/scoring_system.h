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

#ifndef PASTE_ALIGNMENTS_SCORING_SYSTEM_H_
#define PASTE_ALIGNMENTS_SCORING_SYSTEM_H_

#include <array>
#include <string>
#include <sstream>

#include "exceptions.h"
#include "helpers.h"
#include "paste_parameters.h"

namespace paste_alignments {

/// @brief Describes a set of scoring parameter values.
///
struct ScoringParameters {
  /// @brief Reward for a nucleotide match.
  ///
  int reward;

  /// @brief Penalty for a nucleotide mismatch.
  ///
  int penalty;

  /// @brief Cost to open a gap.
  ///
  int open_cost;

  /// @brief Cost to extend a gap.
  ///
  int extend_cost;

  /// @brief Statistical parameter derived from reward, penalty, gap open,
  ///  and gap extension cost.
  ///
  float lambda;

  /// @brief Statistical parameter derived from reward, penalty, gap open,
  ///  and gap extension cost.
  ///
  float k;

  /// @brief Constructs object with the specified values.
  ///
  constexpr ScoringParameters(int reward, int penalty, int open_cost,
                              int extend_cost, float lambda, float k)
      : reward{reward}, penalty{penalty}, open_cost{open_cost},
        extend_cost{extend_cost}, lambda{lambda}, k{k} {}
};

/// @brief Encapsulates scoring parameters, computes raw alignment scores and
///  statistical values associated with it.
///
/// @details By default, the megablsat default parameter values are used
///  (according to [NCBI BLAST manual appendix](https://www.ncbi.nlm.nih.gov/books/NBK279684/)
///  as of May 23rd, 2020).
///
/// For more information on the statistics behind sequence similarity scores,
///  see: [NCBI BLAST tutorial](https://www.ncbi.nlm.nih.gov/BLAST/tutorial/Altschul-1.html).
///  For more information on the scoring parameter values supported by blastn
///  and megablast, see: [NCBI BLAST manual appendix](https://www.ncbi.nlm.nih.gov/books/NBK279684/).
///
/// @invariant `DatabaseSize` is positive. 
/// @invariant Values of the scoring parameters are one of the sets of parameter
///  values listed in `ScoringSystem::kBLASTSupportedScoringParameters`.
///
class ScoringSystem {
 public:
  /// @brief Lists the sets of scoring parameter values supported by objects of
  ///  the `ScoringSystem` class.
  ///
  /// @details BLAST supports a fixed collection of sets of parameter values.
  ///  These values were obtained from the source code of BLAST from the file
  ///  c++/src/algo/blast/core/blast_stat.c in the source code for BLAST+ 2.10
  ///  obtained on May 23rd, 2020 from [the official BLAST download page](https://ftp.ncbi.nlm.nih.gov/blast/executables/blast+/LATEST/).
  ///  (reward, penalty) pairs (2, 7), (2, 5), and (2, 3) can only be applied to
  ///  even scores when calculating an evalue. Any odd score must be rounded down
  ///  to the nearest even number before calculating the evalue.
  ///
  static constexpr std::array<ScoringParameters, 60>
  kBLASTSupportedScoringParameters{
    ScoringParameters{1, 5, 0, 0, 1.39f, 0.747f},
    ScoringParameters{1, 5, 3, 3, 1.39f, 0.747f},
    ScoringParameters{1, 4, 0, 0, 1.383f, 0.738f},
    ScoringParameters{1, 4, 1, 2, 1.36f, 0.67f},
    ScoringParameters{1, 4, 0, 2, 1.26f, 0.43f},
    ScoringParameters{1, 4, 2, 1, 1.35f, 0.61f},
    ScoringParameters{1, 4, 1, 1, 1.22f, 0.35f},
    ScoringParameters{2, 7, 0, 0, 0.69f, 0.73f},
    ScoringParameters{2, 7, 2, 4, 0.68f, 0.67f},
    ScoringParameters{2, 7, 0, 4, 0.63f, 0.43f},
    ScoringParameters{2, 7, 4, 2, 0.675f, 0.62f},
    ScoringParameters{2, 7, 2, 2, 0.61f, 0.35f},
    ScoringParameters{1, 3, 0, 0, 1.374f, 0.711f},
    ScoringParameters{1, 3, 2, 2, 1.37f, 0.70f},
    ScoringParameters{1, 3, 1, 2, 1.35f, 0.64f},
    ScoringParameters{1, 3, 0, 2, 1.25f, 0.42f},
    ScoringParameters{1, 3, 2, 1, 1.34f, 0.60f},
    ScoringParameters{1, 3, 1, 1, 1.21f, 0.34f},
    ScoringParameters{2, 5, 0, 0, 0.675f, 0.65f},
    ScoringParameters{2, 5, 2, 4, 0.67f, 0.59f},
    ScoringParameters{2, 5, 0, 4, 0.62f, 0.39f},
    ScoringParameters{2, 5, 4, 2, 0.67f, 0.61f},
    ScoringParameters{2, 5, 2, 2, 0.56f, 0.32f},
    ScoringParameters{1, 2, 0, 0, 1.28f, 0.46f},
    ScoringParameters{1, 2, 2, 2, 1.33f, 0.62f},
    ScoringParameters{1, 2, 1, 2, 1.30f, 0.52f},
    ScoringParameters{1, 2, 0, 2, 1.19f, 0.34f},
    ScoringParameters{1, 2, 3, 1, 1.32f, 0.57f},
    ScoringParameters{1, 2, 2, 1, 1.29f, 0.49f},
    ScoringParameters{1, 2, 1, 1, 1.14f, 0.26f},
    ScoringParameters{2, 3, 0, 0, 0.55f, 0.21f},
    ScoringParameters{2, 3, 4, 4, 0.63f, 0.42f},
    ScoringParameters{2, 3, 2, 4, 0.615f, 0.37f},
    ScoringParameters{2, 3, 0, 4, 0.55f, 0.21f},
    ScoringParameters{2, 3, 3, 3, 0.615f, 0.37f},
    ScoringParameters{2, 3, 6, 2, 0.63f, 0.42f},
    ScoringParameters{2, 3, 5, 2, 0.625f, 0.41f},
    ScoringParameters{2, 3, 4, 2, 0.61f, 0.35f},
    ScoringParameters{2, 3, 2, 2, 0.515, 0.14f},
    ScoringParameters{3, 4, 6, 3, 0.389f, 0.25f},
    ScoringParameters{3, 4, 5, 3, 0.375f, 0.21f},
    ScoringParameters{3, 4, 4, 3, 0.351f, 0.14f},
    ScoringParameters{3, 4, 6, 2, 0.362f, 0.16f},
    ScoringParameters{3, 4, 5, 2, 0.330f, 0.092f},
    ScoringParameters{3, 4, 4, 2, 0.281f, 0.046f},
    ScoringParameters{4, 5, 0, 0, 0.22f, 0.061f},
    ScoringParameters{4, 5, 6, 5, 0.28f, 0.21f},
    ScoringParameters{4, 5, 5, 5, 0.27f, 0.17f},
    ScoringParameters{4, 5, 4, 5, 0.25f, 0.10f},
    ScoringParameters{4, 5, 3, 5, 0.23f, 0.065f},
    ScoringParameters{1, 1, 3, 2, 1.09f, 0.31f},
    ScoringParameters{1, 1, 2, 2, 1.07f, 0.27f},
    ScoringParameters{1, 1, 1, 2, 1.02f, 0.21f},
    ScoringParameters{1, 1, 0, 2, 0.80f, 0.064f},
    ScoringParameters{1, 1, 4, 1, 1.08f, 0.28f},
    ScoringParameters{1, 1, 3, 1, 1.06f, 0.25f},
    ScoringParameters{1, 1, 2, 1, 0.99f, 0.17f},
    ScoringParameters{3, 2, 5, 5, 0.208f, 0.030f},
    ScoringParameters{5, 4, 10, 6, 0.163f, 0.068f},
    ScoringParameters{5, 4, 8, 6, 0.146f, 0.039f}};

  /// @name Factories:
  ///
  /// @{
  
  /// @brief Creates `ScoringSystem` object with the provided scoring
  ///  parameters.
  ///
  /// @parameter reward Match reward scoring parameter.
  /// @parameter penalty Mismatch penalty scoring parameter.
  /// @parameter open_cost Gap open cost scoring parameter.
  /// @parameter extend_cost Gap extension cost scoring parameter.
  /// @parameter db_size Size of the database associated with the object.
  ///
  /// @details Acts like executing `SetScoringParameters` and `DatabaseSize`
  ///  with the same parameters.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::ScoringError` if
  ///  * Requested set of scoring parameter values is not supported.
  ///  * `db_size` is not positive.
  ///
  static ScoringSystem Create(long db_size, int reward = 1, int penalty = 2,
                              int open_cost = 0, int extend_cost = 0);
  /// @}

  /// @name Constructors:
  ///
  /// @{
  
  /// @brief Copy constructor.
  ///
  ScoringSystem(const ScoringSystem& other) = default;

  /// @brief Move constructor.
  ///
  ScoringSystem(ScoringSystem&& other) noexcept = default;
  /// @}
  
  /// @name Assignment:
  ///
  /// @{
  
  /// @brief Copy assignment.
  ///
  ScoringSystem& operator=(const ScoringSystem& other) = delete;

  /// @brief Move assignment.
  ///
  ScoringSystem& operator=(ScoringSystem&& other) = delete;
  /// @}

  /// @name Modifiers:
  ///
  /// @{
  
  /// @brief Assigns provided scoring parameter values to the object if they
  ///  define a supported set of scoring parameters.
  ///
  /// @parameter reward Match reward scoring parameter.
  /// @parameter penalty Mismatch penalty scoring parameter.
  /// @parameter open_cost Gap open cost scoring parameter.
  /// @parameter extend_cost Gap extension cost scoring parameter.
  ///
  /// @exceptions Basic guarantee. Throws `exceptions::ScoringError` if
  ///  requested set of scoring parameter values is not supported.
  ///
  void SetScoringParameters(int reward, int penalty, int open_cost,
                            int extend_cost);

  /// @brief Sets database size to `value`.
  ///
  /// @parameter value The new value for the database size associated with the
  ///  object.
  ///
  /// @exceptions Strong guarantee. Throws `exceptions::OutOfRange` if `value`
  ///  is not positive.
  ///
  inline void DatabaseSize(long value) {
    helpers::TestPositive(value);
    db_size_ = value;
  }
  /// @}

  /// @name Accessors:
  ///
  /// @{
  
  /// @brief Match reward parameter value used by the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float Reward() const {return reward_;}

  /// @brief Mismatch penalty parameter value used by the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float Penalty() const {return penalty_;}

  /// @brief Gap open cost parameter value used by the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float OpenCost() const {return open_cost_;}

  /// @brief Gap extension cost parameter value used by the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float ExtendCost() const {return extend_cost_;}

  /// @brief Lambda value corresponding to the used scoring parameter values.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float Lambda() const {return lambda_;}

  /// @brief Value of the constant `k` corresponding to the used scoring
  ///  parameter values.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float K() const {return k_;}

  /// @brief Database size associated with the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline long DatabaseSize() const {return db_size_;}
  /// @}

  /// @name Alignment statistics computation:
  ///
  /// @{

  /// @brief Computes alignment's raw score.
  ///
  /// @parameter nident Number of identically matching residue pairs in
  ///  alignment.
  /// @parameter mismatch Number of mismatching residue pairs in alignment.
  /// @parameter gapopen Number of consecutive runs of gap characters on either
  ///  side of the alignment.
  /// @parameter gaps Number of residue pairs aligned with gap characters in the
  ///  alignment.
  ///
  /// @details The raw alignment score is defined by the expression
  ///  `reward * nident - penalty * mismatch - open_cost * gapopen - extend_cost * gaps`.
  ///
  /// @exceptions Strong guarantee.
  ///
  inline float RawScore(int nident, int mismatch, int gapopen, int gaps) const {
    return (reward_ * nident
            - penalty_ * mismatch
            - open_cost_ * gapopen
            - extend_cost_ * gaps);
  }
  
  /// @brief Computes alignment's bitscore.
  ///
  /// @parameter raw_score Alignment's raw score.
  /// @parameter parameters Additional arguments to deal with floating points.
  ///
  /// @details The bitscore is defined by the expression
  ///  `(lambda * score - ln(k)) / ln(2)`. (Reward, penalty) value pairs (2,3),
  ///  (2,5), (2,7) are rounded down to next lower even number if they are odd.
  ///
  /// @exceptions Strong guarantee.
  ///
  float Bitscore(float raw_score, const PasteParameters& parameters) const;

  /// @brief Computes alignment's evalue.
  ///
  /// @parameter raw_score Alignment's raw score.
  /// @parameter qlen Length of query sequence.
  /// @parameter parameters Additional arguments to deal with floating points.
  ///
  /// @details The evalue is defined by the expression
  ///  `k * qlen * db_size * (e ^ (-lambda * score))`. (Reward, penalty) value
  ///  pairs (2,3), (2,5), (2,7) are rounded down to next lower even number if
  ///  they are odd.
  ///
  /// @exceptions Strong guarantee.
  ///  
  double Evalue(float raw_score, int qlen,
                const PasteParameters& parameters) const;
  /// @}

  /// @name Other:
  ///
  /// @{
/*
  /// @brief Compares the object to `other`.
  ///
  /// @exceptions Strong guarantee.
  ///
  bool operator==(const ScoringSystem& other) const;
*/
  /// @brief Returns a descriptive string of the object.
  ///
  /// @exceptions Strong guarantee.
  ///
  std::string DebugString() const;
  /// @}

 private:
  /// @brief Default constructor.
  ///
  ScoringSystem() = default;

  float reward_{1.0f};
  float penalty_{2.0f};
  float open_cost_{0.0f};
  float extend_cost_{2.5f};
  float lambda_{1.28f};
  float k_{0.46f};
  long int db_size_{0};
};

} // namespace paste_alignments

#endif // PASTE_ALIGNMENTS_SCORING_SYSTEM_H_