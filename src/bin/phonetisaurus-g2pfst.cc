/*
 phonetisaurus-g2pfst.cc

 Copyright (c) [2012-], Josef Robert Novak
 All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted #provided that the following conditions
   are met:

   * Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above 
     copyright notice, this list of #conditions and the following 
     disclaimer in the documentation and/or other materials provided 
     with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
   OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#include <fst/fstlib.h>
using namespace std;
#include <include/PhonetisaurusScript.h>
#include <include/util.h>
#include <iomanip>
using namespace fst;

typedef unordered_map<int, vector<PathData> > RMAP;

void PrintPathData (const vector<PathData>& results, string FST_FLAGS_word,
		    const SymbolTable* osyms, bool print_scores = true,
		    bool nlog_probs = true) {
  for (int i = 0; i < results.size (); i++) {
    cout << FST_FLAGS_word << "\t";
    if (print_scores == true) {
      if (nlog_probs == true) 
	cout << results [i].PathWeight << "\t";
      else
	cout << std::setprecision (3) << exp (-results [i].PathWeight) << "\t";
    }
    
    for (int j = 0; j < results [i].Uniques.size (); j++) {
      cout << osyms->Find (results [i].Uniques [j]);
      if (j < results [i].Uniques.size () - 1)
	cout << " ";
    }
    cout << endl;
  }    
}

void EvaluateWordlist (PhonetisaurusScript& decoder, vector<string> corpus,
		       int FST_FLAGS_beam, int FST_FLAGS_nbest, bool FST_FLAGS_reverse,
		       string FST_FLAGS_skip, double FST_FLAGS_thresh, string FST_FLAGS_gsep,
		       bool FST_FLAGS_write_fsts, bool FST_FLAGS_print_scores,
		       bool FST_FLAGS_accumulate, double FST_FLAGS_pmass,
		       bool FST_FLAGS_nlog_probs) {
  for (int i = 0; i < corpus.size (); i++) {
    vector<PathData> results = decoder.Phoneticize (corpus [i], FST_FLAGS_nbest,
						    FST_FLAGS_beam, FST_FLAGS_thresh,
						    FST_FLAGS_write_fsts,
						    FST_FLAGS_accumulate, FST_FLAGS_pmass);
    PrintPathData (results, corpus [i],
		   decoder.osyms_,
		   FST_FLAGS_print_scores,
		   FST_FLAGS_nlog_probs);
  }
}


DEFINE_string (model, "", "Input FST G2P model.");
DEFINE_string (word, "", "Input word to phoneticize.");
DEFINE_string (wordlist, "", "Input wordlist to phoneticize");
DEFINE_string (gsep, "", "Grapheme separator.");
DEFINE_string (skip, "_", "Phoneme skip marker.");
DEFINE_int32 (nbest, 1, "N-best hypotheses to output.");
DEFINE_int32 (beam, 10000, "Decoder beam.");
DEFINE_double (thresh, 99.0, "N-best comparison threshold.");
DEFINE_double (pmass, 0.0, "Percent of probability mass (0.0 < p <= 1.0).");
DEFINE_bool (write_fsts, false, "Write the output FSTs for debugging.");
DEFINE_bool (reverse, false, "Reverse input word.");
DEFINE_bool (print_scores, true, "Print scores in output.");
DEFINE_bool (accumulate, false, "Accumulate weights for unique output prons.");
DEFINE_bool (nlog_probs, true, "Default scores vals are negative logs. "
	     "Otherwise exp (-val).");
int main (int argc, char* argv []) {
  cerr << "GitRevision: " << GIT_REVISION << endl;
  string usage = "phonetisaurus-g2pfst - joint N-gram decoder.\n\n Usage: ";
  set_new_handler (FailedNewHandler);
  PhonetisaurusSetFlags (usage.c_str(), &argc, &argv, false);

  if (FST_FLAGS_model.compare ("") == 0) {
    cerr << "You must supply an FST model to --model" << endl;
    exit (1);
  } else {
    std::ifstream model_ifp (FST_FLAGS_model);
    if (!model_ifp.good ()) {
      cout << "Failed to open --model file '"
	   << FST_FLAGS_model << "'" << endl;
      exit (1);
    }
  }

  if (FST_FLAGS_pmass < 0.0 || FST_FLAGS_pmass > 1) {
    cout << "--pmass must be a float value between 0.0 and 1.0." << endl;
    exit (1);
  }
  if (FST_FLAGS_pmass == 0.0)
    FST_FLAGS_pmass = 99.0;
  else
    FST_FLAGS_pmass = -log (FST_FLAGS_pmass);
  
  bool use_wordlist = false;
  if (FST_FLAGS_wordlist.compare ("") != 0) {
    std::ifstream wordlist_ifp (FST_FLAGS_wordlist);
    if (!wordlist_ifp.good ()) {
      cout << "Failed to open --wordlist file '"
	   << FST_FLAGS_wordlist << "'" << endl;
      exit (1);
    } else {
      use_wordlist = true;
    }
  }

  if (FST_FLAGS_wordlist.compare ("") == 0 && FST_FLAGS_word.compare ("") == 0) {
    cout << "Either --wordlist or --word must be set!" << endl;
    exit (1);
  }

  if (use_wordlist == true) {
    vector<string> corpus;
    LoadWordList (FST_FLAGS_wordlist, &corpus);
    
    PhonetisaurusScript decoder (FST_FLAGS_model, FST_FLAGS_gsep);
    EvaluateWordlist (
	    decoder, corpus, FST_FLAGS_beam, FST_FLAGS_nbest, FST_FLAGS_reverse,
	    FST_FLAGS_skip, FST_FLAGS_thresh, FST_FLAGS_gsep, FST_FLAGS_write_fsts,
	    FST_FLAGS_print_scores, FST_FLAGS_accumulate, FST_FLAGS_pmass,
	    FST_FLAGS_nlog_probs
	  );
  } else {
    PhonetisaurusScript decoder (FST_FLAGS_model, FST_FLAGS_gsep);
    vector<PathData> results = decoder.Phoneticize (
		         FST_FLAGS_word, FST_FLAGS_nbest, FST_FLAGS_beam, FST_FLAGS_thresh,
			 FST_FLAGS_write_fsts, FST_FLAGS_accumulate, FST_FLAGS_pmass
		       );
    PrintPathData (results, FST_FLAGS_word,
		   decoder.osyms_,
		   FST_FLAGS_print_scores,
		   FST_FLAGS_nlog_probs);
  }
  
  return 0;
}
