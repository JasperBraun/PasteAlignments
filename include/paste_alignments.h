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

#ifndef PASTE_ALIGNMENTS_PASTE_ALIGNMENTS_H_
#define PASTE_ALIGNMENTS_PASTE_ALIGNMENTS_H_

#include "alignment.h"
#include "exceptions.h"
#include "helpers.h"
#include "scoring_system.h"

/// @defgroup PasteAlignments-Reference
///
/// @{

/// @brief Library namespace.
///
/// @details All types and functions defined by the library reside inside this
///  namespace.
///
namespace paste_alignments {}

/// @brief Contains library-specific exceptions.
///
/// @details All non-STL exceptions thrown by library functions are derived
///  from `BaseError`.
///
namespace paste_alignments::exceptions {}

/// @brief Contains various helper functions.
///
namespace paste_alignments::helpers {}
/// @}

#endif // PASTE_ALIGNMENTS_PASTE_ALIGNMENTS_H_