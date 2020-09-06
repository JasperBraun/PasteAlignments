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

#include "scoring_system.h"

#include <cmath>
#include <sstream>

namespace paste_alignments {

namespace {

// If `x` is odd, returns the next lower even number; else, does nothng.
// Distance to the nearest even number for `x` to be considered even must not
// exceed `epsilon` times magnitude of the smaller non-zero.
//
float NextLowerEven(float x, float epsilon = 0.05f) {
  float result;
  float modulo{std::abs(std::fmod(x, 2.0f))};
  if (!helpers::FuzzyFloatEquals(0.0f, modulo, epsilon)
      && !helpers::FuzzyFloatEquals(2.0f, modulo, epsilon)) {
    result = 2.0f * (std::floorf(x / 2.0f));
  } else {
    result = x;
  }
  return result;
}

} // namespace

// ScoringSystem::Create
//
ScoringSystem ScoringSystem::Create(long db_size, int reward, int penalty,
                                    int open_cost, int extend_cost) {
  ScoringSystem result;
  result.SetScoringParameters(reward, penalty, open_cost, extend_cost);
  result.DatabaseSize(db_size);
  return result;
}

// ScoringSystem::SetScoringParameters
//
void ScoringSystem::SetScoringParameters(int reward, int penalty, int open_cost,
                                         int extend_cost) {
  std::stringstream error_message;
  for (const ScoringParameters& parameters : kBLASTSupportedScoringParameters) {
    if (parameters.reward == reward
        && parameters.penalty == penalty
        && parameters.open_cost == open_cost
        && parameters.extend_cost == extend_cost) {
      reward_ = static_cast<float>(reward);
      penalty_ = static_cast<float>(penalty);
      open_cost_ = static_cast<float>(open_cost);
      if (open_cost == 0 && extend_cost == 0) {
        extend_cost_ = (reward / 2.0f) + penalty;
      } else {
        extend_cost_ = static_cast<float>(extend_cost);
      }
      lambda_ = parameters.lambda;
      k_ = parameters.k;
      return;
    }
  }
  error_message << "Scoring system defined by"
                << " (match-reward = " << reward
                << ", mismatch-penalty = " << penalty
                << ", gap-open-cost = " << open_cost
                << ", gap-extension-cost = " << extend_cost
                << ") requested, but not supported.";
  throw exceptions::ScoringError(error_message.str());
}

// ScoringSystem::Bitscore
//
float ScoringSystem::Bitscore(float raw_score,
                              const PasteParameters& parameters) const {
  if (reward_ == 2 && (penalty_ == 3 || penalty_ == 5 || penalty_ == 7)) {
    return ((lambda_ * NextLowerEven(raw_score, parameters.float_epsilon)
             - std::log(k_))
            / std::log(2.0f));
  } else {
    return ((lambda_ * raw_score - std::log(k_)) / std::log(2.0f));
  }
}

// ScoringSystem::Evalue
//
double ScoringSystem::Evalue(float raw_score, int qlen,
                             const PasteParameters& parameters) const {
  double score;
  if (reward_ == 2 && (penalty_ == 3 || penalty_ == 5 || penalty_ == 7)) {
    score = static_cast<double>(NextLowerEven(raw_score,
                                              parameters.float_epsilon));
  } else {
    score = static_cast<double>(raw_score);
  }
  return (static_cast<double>(k_)
          * static_cast<double>(qlen)
          * static_cast<double>(db_size_)
          * std::exp((-1.0) * static_cast<double>(lambda_) * score));
}

/*
// ScoringSystem::operator==
//
bool ScoringSystem::operator==(const ScoringSystem& other) const {
  return(reward_ == other.reward_
         && penalty_ == other.penalty_
         && open_cost_ == other.open_cost_
         && helpers::FuzzyFloatEquals(extend_cost_, other.extend_cost_)
         && helpers::FuzzyFloatEquals(lambda_, other.lambda_)
         && helpers::FuzzyFloatEquals(k_, other.k_)
         && db_size_ == other.db_size_);
}
*/

// ScoringSystem::DebugString()
//
std::string ScoringSystem::DebugString() const {
  std::stringstream ss;
  ss << "{reward: " << reward_ << ", penalty: " << penalty_
     << ", gap-open-cost: " << open_cost_ << ", gap-extension-cost: "
     << extend_cost_ << ", lambda: " << lambda_ << ", k: " << k_
     << ", db_size: " << db_size_ << '}';
  return ss.str();
}

} // namespace paste_alignments