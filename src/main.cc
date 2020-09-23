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

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "arg_parse_convert.h"
#include "paste_alignments.h"

namespace {

const char* kUsageMessage{
    "\nusage: paste_alignments [options] --db_size INTEGER INPUT_FILE [OUTPUT_FILE]\n"};

const char* kVersionMessage{
    "\nPasteAlignments v1.0.0"
    "\nCopyright (c) 2020 Jasper Braun"};


// Initializes `ParameterMap` object for argument parsing.
//
arg_parse_convert::ParameterMap InitParameters() {
  arg_parse_convert::ParameterMap parameter_map;
  parameter_map(arg_parse_convert::Parameter<std::string>::Positional(
                   arg_parse_convert::converters::StringIdentity,
                   "input_file", 0)
                .MinArgs(1).MaxArgs(1).Placeholder("INPUT_FILE")
                .Description(
                    "Tab-delimited HSP table as returned by BLAST with option"
                    " `-outfmt '6 qseqid sseqid qstart qend sstart send nident"
                    " mismatch gapopen gaps qlen slen length qseq sseq. If"
                    " executing in blind mode, the last two columns can be left"
                    " out. Each alignment is considered to be on the minus"
                    " strand if it's subject end coordinate precedes its"
                    " subject start coordinate. Fields in excess of 13 (11 if"
                    " in blind mode) are ignored."))

               (arg_parse_convert::Parameter<std::string>::Positional(
                    arg_parse_convert::converters::StringIdentity,
                    "output_file", 1)
                .MinArgs(0).MaxArgs(1).Placeholder("OUTPUT_FILE")
                .Description(
                    "Tab-delimited HSP table with columns: qseqid sseqid qstart"
                    " qend sstart send nident mismatch gapopen gaps qlen slen"
                    " length qseq sseq pident score bitscore evalue nmatches"
                    " rows, where nmatches is the number of N-N matches and"
                    " 'rows' is a comma-separated list of row numbers for the"
                    " alignments from the input file that, when pasted"
                    " together, constitute the output alignments. If executing"
                    " in blind mode, the qseq and sseq columns are omitted. For"
                    " alignments on the minus strand, the subject end"
                    " coordinate precedes its subject start coordinate."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"d", "db", "db_size"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .Description(
                    "Size of the database used for the BLAST search. Required"
                    " for the computation of evalues."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"g", "gap", "gap_tolerance"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .AddDefault("4")
                .Description(
                    "Maximum gap length allowed to be introduced through"
                    " pasting."))

               (arg_parse_convert::Parameter<float>::Keyword(
                    arg_parse_convert::converters::stof,
                    {"final_pident", "final_pident_threshold"})
                .MinArgs(1).MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.0")
                .Description(
                    "Percent identity threshold alignments must satisfy to be"
                    " included in the output."))

               (arg_parse_convert::Parameter<float>::Keyword(
                    arg_parse_convert::converters::stof,
                    {"final_score", "final_score_threshold"})
                .MinArgs(1).MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.0")
                .Description(
                    "Raw score threshold alignments must satisfy to be included"
                    " in the output."))

               (arg_parse_convert::Parameter<float>::Keyword(
                    arg_parse_convert::converters::stof,
                    {"intermediate_pident", "intermediate_pident_threshold"})
                .MinArgs(1).MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.0")
                .Description(
                    "Percent identity threshold that must be satisfied during"
                    " pasting."))

               (arg_parse_convert::Parameter<float>::Keyword(
                    arg_parse_convert::converters::stof,
                    {"intermediate_score", "intermediate_score_threshold"})
                .MinArgs(1).MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.0")
                .Description(
                    "Raw score threshold that must be satisfied during"
                    " pasting."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"r", "reward", "match_reward"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .AddDefault("1")
                .Description(
                    "Match reward used to compute score, bitscore, and evalue."
                    " Only a fixed set of values is supported."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"p", "penalty", "mismatch_penalty"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .AddDefault("2")
                .Description(
                    "Mismatch penalty used to compute score, bitscore, and"
                    " evalue. Only a fixed set of values is supported."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"o", "gapopen", "gapopen_cost"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .AddDefault("0")
                .Description(
                    "Gap opening cost used to compute score, bitscore, and"
                    " evalue. Only a fixed set of values is supported. For"
                    " megablast scoring parameters set this value to 0."))

               (arg_parse_convert::Parameter<int>::Keyword(
                    arg_parse_convert::converters::stoi,
                    {"e", "gapextend", "gapextend_cost"})
                .MinArgs(1).MaxArgs(1).Placeholder("INTEGER")
                .AddDefault("0")
                .Description(
                    "Gap extension cost used to compute score, bitscore, and"
                    " evalue. Only a fixed set of values is supported. For"
                    " megablast scoring parameters set this value to 0."))

               (arg_parse_convert::Parameter<std::string>::Keyword(
                    arg_parse_convert::converters::StringIdentity,
                    {"y", "summary", "summary_file"})
                .MaxArgs(1).Placeholder("SUMMARY_FILE")
                .Description(
                    "Print overall statistics in JSON format with 1: number of"
                    " alignments, 2: number of pastings performed, 3: average"
                    " alignment length, 4: average percent identity, 5: average"
                    " raw alignment score, 6: average bitscore, 7: average"
                    " evalue, 8: average number of unknown N-N matches (which"
                    " are treated as mismatches."))

               (arg_parse_convert::Parameter<std::string>::Keyword(
                    arg_parse_convert::converters::StringIdentity,
                    {"s", "stats", "stats_file"})
                .MaxArgs(1).Placeholder("STATS_FILE")
                .Description(
                    "Print tab-separated data with columns: 1: query sequence"
                    " identifier, 2: subject sequence identifier, 3:"
                    " number of alignments, 4: number of pastings performed, 5:"
                    " average alignment length, 6: average percent identity, 7:"
                    " average raw alignment score, 8: average bitscore, 9:"
                    " average evalue, 10: average number of unknown N-N matches"
                    " (which are treated as mismatches."))

               (arg_parse_convert::Parameter<std::string>::Keyword(
                    arg_parse_convert::converters::StringIdentity,
                    {"c", "config", "configuration_file"})
                .MaxArgs(1).Placeholder("CONFIGURATION_FILE")
                .Description(
                    "Read parameters from configuration file."))

               (arg_parse_convert::Parameter<float>::Keyword(
                    arg_parse_convert::converters::stof,
                    {"float_epsilon"})
                .MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.01")
                .Description(
                    "Used for floating point comparison of the C++ `float` data"
                    " type."))

               (arg_parse_convert::Parameter<double>::Keyword(
                    arg_parse_convert::converters::stod,
                    {"double_epsilon"})
                .MaxArgs(1).Placeholder("FLOAT")
                .AddDefault("0.01")
                .Description(
                    "Used for floating point comparison of the C++ `double`"
                    " data type."))

               (arg_parse_convert::Parameter<bool>::Flag(
                    {"blind", "blind_mode"})
                .Description(
                    "Disregard actual sequences during pasting. No alignment"
                    " sequences are read or constructed during pasting in this"
                    " mode. However query and subject coordinates, number of"
                    " identities, mismatches, gap openings, and gap extensions"
                    " (and thus percent identity, score, bitscore, and evalue)"
                    " are still computed."))

               (arg_parse_convert::Parameter<bool>::Flag(
                    {"enforce_avg_score", "enforce_average_score"})
                .Description(
                    "Paste alignments only when the pasted score is at least as"
                    " large as the average score of the two alignments."))

               (arg_parse_convert::Parameter<bool>::Flag(
                    {"h", "help"})
                .Description("Print this help message and exit."))

               (arg_parse_convert::Parameter<bool>::Flag(
                    {"version"})
                .Description("Print the software's version and exit."));

  return parameter_map;
}

// Parses arguments argc, argv and contained configuration file, if any.
//
arg_parse_convert::ArgumentMap ParseArguments(int argc, const char** argv) {
  std::vector<std::string> additional_arguments;
  std::stringstream error_message;
  arg_parse_convert::ParameterMap parameter_map{InitParameters()};
  arg_parse_convert::ArgumentMap argument_map{std::move(parameter_map)};
  additional_arguments = arg_parse_convert::ParseArgs(argc, argv,
                                                      argument_map);

  if (!additional_arguments.empty()) {
    // Some arguments couldn't be assiged to parameters.
    error_message << "Invalid argument: " << additional_arguments.at(0)
                  << std::endl;
    throw arg_parse_convert::exceptions::ArgumentParsingError(
        error_message.str());
  } else if (argument_map.HasArgument("configuration_file")) {
    std::ifstream ifs{argument_map.GetValue<std::string>("configuration_file")};
    if (ifs.is_open()) {
      additional_arguments = arg_parse_convert::ParseFile(ifs, argument_map);
    } else {
      error_message << "Unable to open configuration file: "
                    << argument_map.GetValue<std::string>("configuration_file")
                    << std::endl;
      throw arg_parse_convert::exceptions::ArgumentParsingError(
          error_message.str());
    }

    if (!additional_arguments.empty()) {
      // Some arguments couldn't be assiged to parameters.
      error_message << "Invalid argument: " << additional_arguments.at(0)
                    << std::endl;
      throw arg_parse_convert::exceptions::ArgumentParsingError(
          error_message.str());
    }
  }
  argument_map.SetDefaultArguments();
  return argument_map;
}

// Converts arguments stored in `argument_map` into `PasteParameters` object.
//
paste_alignments::PasteParameters GetPasteParameters(
    arg_parse_convert::ArgumentMap argument_map) {
  paste_alignments::PasteParameters result;

  // Pasting parameters.
  result.gap_tolerance = argument_map.GetValue<int>("gap_tolerance");
  result.intermediate_pident_threshold = argument_map.GetValue<float>(
      "intermediate_pident");
  result.intermediate_score_threshold = argument_map.GetValue<float>(
      "intermediate_score");
  result.final_pident_threshold = argument_map.GetValue<float>("final_pident");
  result.final_score_threshold = argument_map.GetValue<float>("final_score");
  result.blind_mode = argument_map.IsSet("blind_mode");
  result.enforce_average_score = argument_map.IsSet("enforce_average_score");

  // Scoring parameters.
  result.reward = argument_map.GetValue<int>("reward");
  result.penalty = argument_map.GetValue<int>("penalty");
  result.open_cost = argument_map.GetValue<int>("gapopen");
  result.extend_cost = argument_map.GetValue<int>("gapextend");
  result.db_size = argument_map.GetValue<int>("db_size");

  // Input/Output.
  result.input_filename = argument_map.GetValue<std::string>("input_file");
  if (argument_map.HasArgument("output_file")) {
    result.output_filename = argument_map.GetValue<std::string>("output_file");
  }
  if (argument_map.HasArgument("summary_file")) {
    result.summary_filename = argument_map.GetValue<std::string>("summary_file");
  }
  if (argument_map.HasArgument("stats_file")) {
    result.stats_filename = argument_map.GetValue<std::string>("stats_file");
  }

  // Other.
  result.float_epsilon = argument_map.GetValue<float>("float_epsilon");
  result.double_epsilon = argument_map.GetValue<double>("double_epsilon");

  return result;
}

// Reads input file, pastes alignments, prints pasted alignments as well as
// summary and descriptive statistics, if desired, into output files.
//
void PasteAlignments(
    const paste_alignments::PasteParameters& paste_parameters) {

  // Input file.
  int num_fields = 13;
  if (paste_parameters.blind_mode) {
    num_fields -= 2;
  }
  std::unique_ptr<std::ifstream> inputs_ifs{
      new std::ifstream{paste_parameters.input_filename}};
  paste_alignments::AlignmentReader reader{
      paste_alignments::AlignmentReader::FromIStream(std::move(inputs_ifs),
                                                     num_fields)};
  // Scoring system.
  paste_alignments::ScoringSystem scoring_system{
      paste_alignments::ScoringSystem::Create(
          paste_parameters.db_size, paste_parameters.reward,
          paste_parameters.penalty, paste_parameters.open_cost,
          paste_parameters.extend_cost)};
  // Output file.
  std::ofstream alignments_ofs;
  if (!paste_parameters.output_filename.empty()) {
    alignments_ofs.open(paste_parameters.output_filename);
  }

  paste_alignments::StatsCollector stats_collector;
  while (!reader.EndOfData()) {
    paste_alignments::AlignmentBatch batch = reader.ReadBatch(scoring_system,
                                                              paste_parameters);
    batch.PasteAlignments(scoring_system, paste_parameters);
    if (!paste_parameters.stats_filename.empty()) {
      stats_collector.CollectStats(batch);
    }
    if (!paste_parameters.output_filename.empty()) {
      paste_alignments::WriteBatch(std::move(batch), alignments_ofs);
    } else {
      paste_alignments::WriteBatch(std::move(batch), std::cout);
    }
  }
  if (!paste_parameters.output_filename.empty()) {
    alignments_ofs.close();
  }

  // Print summary
  if (!paste_parameters.stats_filename.empty()) {
    std::ofstream stats_ofs{paste_parameters.stats_filename};
    paste_alignments::PasteStats summary{stats_collector.WriteData(stats_ofs)};
    stats_ofs.close();
    if (!paste_parameters.summary_filename.empty()) {
      std::ofstream summary_ofs{paste_parameters.summary_filename};
      summary_ofs << "{\n"
                  << "\t\"num_alignments\": " << summary.num_alignments << ",\n"
                  << "\t\"num_pastings\": " << summary.num_pastings << ",\n"
                  << "\t\"average_length\": " << summary.average_length << ",\n"
                  << "\t\"average_pident\": " << summary.average_pident << ",\n"
                  << "\t\"average_score\": " << summary.average_score << ",\n"
                  << "\t\"average_bitscore\": " << summary.average_bitscore << ",\n"
                  << "\t\"average_evalue\": " << summary.average_evalue << ",\n"
                  << "\t\"average_nmatches\": " << summary.average_nmatches << '\n'
                  << "}\n";
      summary_ofs.close();
    }
  }
}

} // namespace

int main(int argc, const char** argv) {
  
  try {
    // Parse command line (and configuration file, if any).
    arg_parse_convert::ArgumentMap argument_map{ParseArguments(argc, argv)};

    // Take care of help/version flags.
    if (argument_map.IsSet("help")) {
      std::string help_string{
          arg_parse_convert::FormattedHelpString(argument_map.Parameters(),
                                                 kUsageMessage,
                                                 kVersionMessage)};
      std::cout << help_string << std::endl;
      return 0;
    }
    if (argument_map.IsSet("version")) {
      std::cout << kVersionMessage << std::endl;
      return 0;
    }

    // Ensure required parameters have arguments.
    std::vector<std::string> unfilled_parameters;
    unfilled_parameters = argument_map.GetUnfilledParameters();
    if (!unfilled_parameters.empty()) {
      std::cerr << "Missing argument for parameter: "
                << unfilled_parameters.at(0) << ".\n" << kUsageMessage
                << std::endl;
      return 1;
    }

    // Paste alignments.
    paste_alignments::PasteParameters paste_parameters{
        GetPasteParameters(std::move(argument_map))};
    PasteAlignments(paste_parameters);

  // Argument parsing errors.
  } catch (const arg_parse_convert::exceptions::BaseError& e) {
    std::cerr << "Error while parsing arguments. Exception message: "
              << e.what() << '\n' << kUsageMessage << std::endl;
    return 1;

  // Computation errors.
  } catch (const paste_alignments::exceptions::BaseException& e) {
    std::cerr << "Error while pasting alignments. Exception message: "
              << e.what() << '\n' << kUsageMessage << std::endl;
    return 1;
  
  // Unexpected errors.
  } catch (const std::exception& e) {
    std::cerr << "Something went wrong. Exception message: " << e.what()
              << std::endl;
    return 1;
  }

  return 0;
}