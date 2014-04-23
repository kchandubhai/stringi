/* This file is part of the 'stringi' package for R.
 * Copyright (c) 2013-2014, Marek Gagolewski and Bartek Tartanus
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "stri_stringi.h"
#include "stri_container_utf8.h"
#include "stri_container_utf16.h"
#include "stri_container_usearch.h"
#include "stri_container_bytesearch.h"
#include "stri_container_integer.h"
#include "stri_container_logical.h"
#include <deque>
#include <utility>
#include <unicode/brkiter.h>
#include <unicode/rbbi.h>
using namespace std;



/** Extract words using a BreakIterator
 *
 * @param str character vector
 * @param locale identifier
 * @return character vector
 *
 * @version 0.2-2 (Marek Gagolewski, 2014-04-23)
 * 
 */
SEXP stri_extract_words(SEXP str, SEXP locale)
{
   str = stri_prepare_arg_string(str, "str");
   const char* qloc = stri__prepare_arg_locale(locale, "locale", true);
   Locale loc = Locale::createFromName(qloc);

   R_len_t str_length = LENGTH(str);
   R_len_t vectorize_length = str_length;

   RuleBasedBreakIterator* briter = NULL;
   UText* str_text = NULL;
   STRI__ERROR_HANDLER_BEGIN
   
   UErrorCode status = U_ZERO_ERROR;
   briter = (RuleBasedBreakIterator*)BreakIterator::createWordInstance(loc, status);
   if (U_FAILURE(status))
      throw StriException(status);
   
   StriContainerUTF8 str_cont(str, vectorize_length);

   SEXP ret;
   STRI__PROTECT(ret = Rf_allocVector(VECSXP, vectorize_length));

   int last_boundary = -1;
   for (R_len_t i = 0; i < vectorize_length; ++i)
   {
      if (str_cont.isNA(i)) {
         SET_VECTOR_ELT(ret, i, stri__vector_NA_strings(1));
         continue;
      }

      // get the current string
      UErrorCode status = U_ZERO_ERROR;
      const char* str_cur_s = str_cont.get(i).c_str();
      str_text = utext_openUTF8(str_text, str_cur_s, str_cont.get(i).length(), &status);
      if (U_FAILURE(status))
         throw StriException(status);
      briter->setText(str_text, status);
      if (U_FAILURE(status))
         throw StriException(status);

      deque< pair<R_len_t,R_len_t> > occurences;
      R_len_t match, last_match = briter->first();
      while ((match = briter->next()) != BreakIterator::DONE) {
         int breakType = briter->getRuleStatus();
         if (breakType != UBRK_WORD_NONE)
            occurences.push_back(pair<R_len_t, R_len_t>(last_match, match));
         last_match = match;
      }

      R_len_t noccurences = (R_len_t)occurences.size();
      if (noccurences <= 0) {
         SEXP ans;
         STRI__PROTECT(ans = Rf_allocVector(STRSXP, 1));
         SET_STRING_ELT(ans, 0, Rf_mkCharLen("", 0));
         SET_VECTOR_ELT(ret, i, ans);
         STRI__UNPROTECT(1);
         continue;
      }

      SEXP ans;
      STRI__PROTECT(ans = Rf_allocVector(STRSXP, noccurences));
      deque< pair<R_len_t,R_len_t> >::iterator iter = occurences.begin();
      for (R_len_t j = 0; iter != occurences.end(); ++iter, ++j) {
         SET_STRING_ELT(ans, j, Rf_mkCharLenCE(str_cur_s+(*iter).first,
            (*iter).second-(*iter).first, CE_UTF8));
      }
      SET_VECTOR_ELT(ret, i, ans);
      STRI__UNPROTECT(1);
   }

   if (briter) { delete briter; briter = NULL; }
   if (str_text) { utext_close(str_text); str_text = NULL; }
   STRI__UNPROTECT_ALL
   return ret;
   STRI__ERROR_HANDLER_END({
      if (briter) { delete briter; briter = NULL; }
      if (str_text) { utext_close(str_text); str_text = NULL; }
   })
}