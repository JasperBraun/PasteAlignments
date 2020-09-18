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

#include "alignment_reader.h"

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_COLOUR_NONE
#include "catch.h"

#include "string_conversions.h" // include after catch.h

#include <limits>

#include "exceptions.h"

// AlignmentReader tests
//
// Test correctness for:
// * FromFile
// * ReadBatch
//
// Test exceptions for:
// * FromFile
// * ReadBatch

namespace paste_alignments {

namespace test {

const std::string kValidInput{
    "qseq1\tsseq1\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "qseq1\tsseq1\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "qseq1\tsseq1\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "qseq1\tsseq1\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\n"
    "qseq1\tsseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "qseq1\tsseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "qseq1\tsseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "qseq1\tsseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\n"
    "qseq2\tsseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "qseq2\tsseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "qseq2\tsseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq2\tsseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "qseq2\tsseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\n"
    "sseq2\tqseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "sseq2\tqseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "sseq2\tqseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "sseq2\tqseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "sseq2\tqseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\n"
    "qseq1\tsseq1\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "qseq1\tsseq1\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "qseq1\tsseq1\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq1\tsseq1\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "qseq1\tsseq1\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\n"
    "qseq4\tsseq4\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\n"
    "qseq4\tsseq4\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\n"
    "qseq4\tsseq4\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\n"
    "qseq4\tsseq4\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\n"
    "qseq4\tsseq4\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT"};

const std::string kValidInputAdditionalColumns{
    "qseq1\tsseq1\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq2\tsseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "sseq2\tqseq2\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq1\tsseq1\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t125\t1101\t1125\t24\t1\t0\t0\t10000\t100000\tGCCCCAAAATTCCCCAAAATTCCCC\tACCCCAAAATTCCCCAAAATTCCCC\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t120\t1131\t1150\t20\t0\t0\t0\t10000\t100000\tCCCCAAAATTCCCCAAAATT\tCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t150\t1001\t1050\t40\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t110\t2111\t2120\t10\t0\t0\t0\t10000\t100000\tCCCCAAAATT\tCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t125\t1111\t1135\t20\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATTCCCCAAAATT\tAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t140\t1121\t1160\t30\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT\tAAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t115\t1096\t1110\t10\t5\t0\t0\t10000\t100000\tGGGGGCCCCAAAATT\tAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t135\t101\t135\t20\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t120\t201\t220\t10\t10\t0\t0\t10000\t100000\tGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,\n"
    "qseq4\tsseq4\t101\t125\t2101\t2125\t10\t15\t0\t0\t10000\t100000\tGGGGGGGGGGGGGGGCCCCAAAATT\tAAAAAAAAAAAAAAACCCCAAAATT\t1.23e-45\tfoo_bar-baz,"};

// Creates a vector of alignments from `alignment_input_data` with consecutive
// run of id's starting at `start_id`.
//
std::vector<Alignment> MakeAlignments(
    const std::vector<std::vector<std::string>>& alignment_input_data,
    int start_id, const ScoringSystem& scoring_system,
    const PasteParameters& paste_parameters) {
  std::vector<Alignment> result;
  int id{start_id};
  std::vector<std::string_view> fields;
  for (const std::vector<std::string>& row : alignment_input_data) {
    fields.clear();
    for (const std::string& field : row) {
      fields.push_back(std::string_view{field});
    }
    result.push_back(Alignment::FromStringFields(id, fields, scoring_system,
                     paste_parameters));
    ++id;
  }
  return result;
}

namespace {

SCENARIO("Test correctness of AlignmentReader::FromFile.",
         "[AlignmentReader][FromIStream][exceptions]") {

  THEN("Valid input does not cause exception.") {
    std::unique_ptr<std::istream> is{new std::stringstream{kValidInput}};
    CHECK_NOTHROW(AlignmentReader::FromIStream(std::move(is)));
  }

  THEN("Negative number of fields causes exception.") {
    std::unique_ptr<std::istream> is{new std::stringstream{kValidInput}};
    int num_fields = GENERATE(take(5, random(-100, -1)));
    CHECK_THROWS_AS(AlignmentReader::FromIStream(std::move(is), num_fields),
                    exceptions::OutOfRange);
  }

  THEN("An empty input stream causes exception.") {
    std::unique_ptr<std::istream> is{new std::stringstream};
    CHECK_THROWS_AS(AlignmentReader::FromIStream(std::move(is)),
                    exceptions::ReadError);
  }

  THEN("No tab characters in first line of input stream cause exception.") {
    std::string invalid_input{"Some string without tab characters.\n"};
    invalid_input.append(kValidInput);
    std::unique_ptr<std::istream> is{new std::stringstream{invalid_input}};
    CHECK_THROWS_AS(AlignmentReader::FromIStream(std::move(is)),
                    exceptions::ReadError);
  }

  THEN("Single tab character in first line of input stream causes exception.") {
    std::string invalid_input{"Some string with single \t character.\n"};
    invalid_input.append(kValidInput);
    std::unique_ptr<std::istream> is{new std::stringstream{invalid_input}};
    CHECK_THROWS_AS(AlignmentReader::FromIStream(std::move(is)),
                    exceptions::ReadError);
  }
}

SCENARIO("Test correctness of AlignmentReader::ReadBatch.",
         "[AlignmentReader][ReadBatch][correctness]") {
  ScoringSystem scoring_system
      = GENERATE(ScoringSystem::Create(100000l, 1, 2, 1, 1),
                 ScoringSystem::Create(10000000l, 2, 3, 0, 0));

  float epsilon = GENERATE(0.05f, 0.15f);
  PasteParameters paste_parameters;
  paste_parameters.float_epsilon = epsilon;

  GIVEN("Expected input data.") {
    // Pairs of identifiers in their order of appearance in input stream.
    std::vector<std::pair<std::string, std::string>> sequence_identifiers{
        {"qseq1", "sseq1"}, {"qseq1", "sseq2"}, {"qseq2", "sseq2"},
        {"sseq2", "qseq2"}, {"qseq1", "sseq1"}, {"qseq4", "sseq4"}
    };

    // Alignments are the same for each pair of sequence identifiers.
    std::vector<std::vector<std::string>> alignment_input_data{
        {"101", "125", "1101", "1125", "24", "1", "0", "0", "10000", "100000", "GCCCCAAAATTCCCCAAAATTCCCC", "ACCCCAAAATTCCCCAAAATTCCCC"},
        {"101", "120", "1131", "1150", "20", "0", "0", "0", "10000", "100000", "CCCCAAAATTCCCCAAAATT", "CCCCAAAATTCCCCAAAATT"},
        {"101", "150", "1001", "1050", "40", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "110", "2111", "2120", "10", "0", "0", "0", "10000", "100000", "CCCCAAAATT", "CCCCAAAATT"},
        {"101", "125", "1111", "1135", "20", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATTCCCCAAAATT", "AAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "140", "1121", "1160", "30", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATTCCCCAAAATTCCCCAAAATT", "AAAAAAAAAACCCCAAAATTCCCCAAAATTCCCCAAAATT"},
        {"101", "115", "1096", "1110", "10", "5", "0", "0", "10000", "100000", "GGGGGCCCCAAAATT", "AAAAACCCCAAAATT"},
        {"101", "135", "101", "135", "20", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATTCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATTCCCCAAAATT"},
        {"101", "120", "201", "220", "10", "10", "0", "0", "10000", "100000", "GGGGGGGGGGCCCCAAAATT", "AAAAAAAAAACCCCAAAATT"},
        {"101", "125", "2101", "2125", "10", "15", "0", "0", "10000", "100000", "GGGGGGGGGGGGGGGCCCCAAAATT", "AAAAAAAAAAAAAAACCCCAAAATT"}
    };

    std::vector<AlignmentBatch> expected_batches;
    for (int i = 0; i < sequence_identifiers.size(); ++i) {
      AlignmentBatch batch{
          sequence_identifiers.at(i).first,
          sequence_identifiers.at(i).second};
      batch.ResetAlignments(MakeAlignments(alignment_input_data, 1 + 10 * i,
                                           scoring_system, paste_parameters),
                            paste_parameters);
      expected_batches.emplace_back(std::move(batch));
    }

    WHEN("Input stream contains data exactly.") {
      std::unique_ptr<std::istream> is{new std::stringstream{kValidInput}};
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is))};

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch computed_batch{reader.ReadBatch(scoring_system,
                                                         paste_parameters)};
          CHECK(computed_batch == expected_batches.at(i));
        }
      }
    }

    WHEN("Input stream has exact number of columns and trailing newline.") {
      std::string with_trailing_newline{kValidInput};
      with_trailing_newline.push_back('\n');
      std::unique_ptr<std::istream> is{
          new std::stringstream{with_trailing_newline}};
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is))};

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch computed_batch{reader.ReadBatch(scoring_system,
                                                         paste_parameters)};
          CHECK(computed_batch == expected_batches.at(i));
        }
      }
    }

    WHEN("Input stream has additional columns.") {
      std::unique_ptr<std::istream> is{
          new std::stringstream{kValidInputAdditionalColumns}};
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is))};

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch computed_batch{reader.ReadBatch(scoring_system,
                                                         paste_parameters)};
          CHECK(computed_batch == expected_batches.at(i));
        }
      }
    }

    WHEN("Input stream has additional columns and trailing newline.") {
      std::string with_trailing_newline{kValidInputAdditionalColumns};
      with_trailing_newline.push_back('\n');
      std::unique_ptr<std::istream> is{
          new std::stringstream{with_trailing_newline}};
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is))};

      THEN("Each run of equal first two columns will constitute one batch.") {
        for (int i = 0; i < sequence_identifiers.size(); ++i) {
          AlignmentBatch computed_batch{reader.ReadBatch(scoring_system,
                                                         paste_parameters)};
          CHECK(computed_batch == expected_batches.at(i));
        }
      }
    }
  }
}

SCENARIO("Test exceptions thrown by AlignmentReader::ReadBatch.",
         "[AlignmentReader][ReadBatch][exceptions]") {
  ScoringSystem scoring_system
      = GENERATE(ScoringSystem::Create(100000l, 1, 2, 1, 1),
                 ScoringSystem::Create(10000000l, 2, 3, 0, 0));

  float epsilon = GENERATE(0.05f, 0.15f);
  PasteParameters paste_parameters;
  paste_parameters.float_epsilon = epsilon;

  GIVEN("A valid input stream.") {
    std::unique_ptr<std::istream> is{new std::stringstream{kValidInput}};

    THEN("Call when already at the end of the data causes exception.") {
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is))};
      while (!reader.EndOfData()) {
        reader.ReadBatch(scoring_system, paste_parameters);
      }

      assert(reader.EndOfData());
      CHECK_THROWS_AS(reader.ReadBatch(scoring_system, paste_parameters),
                      exceptions::ReadError);
    }

    THEN("Expecting more fields than given in the file causes exception.") {
      AlignmentReader reader{AlignmentReader::FromIStream(std::move(is), 14)};
      CHECK_THROWS_AS(reader.ReadBatch(scoring_system, paste_parameters),
                      exceptions::ReadError);
    }
  }
}

} // namespace

} // namespace test

} // namespace paste_alignments