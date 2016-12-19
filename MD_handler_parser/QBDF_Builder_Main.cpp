#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <app/app.h>
#include <ext/hash_map>
#include <Data/QBDF_Builder.h>
#include <Data/NYSE_Imbalance_QBDF_Builder.h>
#include <Data/LSE_Indicative_QBDF_Builder.h>
#include <Data/MDSUB_QBDF_Builder.h>
#include <Data/NYSETAQ_QBDF_Builder.h>
//#include <Data/PITCH_QBDF_Builder.h>
#include <Data/QBT_ARCA_QBDF_Builder.h>
#include <Data/QBT_ITCH41_QBDF_Builder.h>
#include <Data/ReutersRDF_QBDF_Builder.h>
#include <Data/WombatCSV_QBDF_Builder.h>

using namespace hf_core;
using namespace std;

// ----------------------------------------------------------------
// Usage
// ----------------------------------------------------------------
void usage(int argc, char ** argv)
{
#ifdef CW_DEBUG
  printf("Usage: %s -create -date YYYYMMDD -source SOURCE_NAME", argv[0]);
#endif
#ifdef CW_DEBUG
  printf(" -quote RAW_QUOTE_FILE -trade RAW_TRADE_FILE");
#endif
#ifdef CW_DEBUG
  printf(" [-start HHMM] [-end HHMM] [-summ_start HHMM] [-summ_end HHMM]\n\n");
#endif
  exit(1);
}

// ----------------------------------------------------------------
// Main driver
// ----------------------------------------------------------------
int main(int argc, char **argv)
{

  if (argc == 1 ) {
    usage(argc, argv);
  }

  int i = 1;
  int argsLeft = argc-1;

  string quote_file, trade_file, date, output_path, source_name;
  bool create_binary_files = false;
  bool sort_binary_files = false;
  bool create_summary_files = false;
  bool compress_files = false;
  bool late_summary = false;
  int compression_level = 9;

  string sids;
  int interval_secs = -1;
  string start_hhmm("0000");
  string end_hhmm("2359");
  string summ_start_hhmm("0929");
  string summ_end_hhmm("1631");
  string region("na");
  string version("1.1");

  while (argsLeft) {
    if (!strcmp(argv[i],"-quote")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      quote_file = string(argv[i]);
    } else if (!strcmp(argv[i],"-trade")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      trade_file = string(argv[i]);
    } else if (!strcmp(argv[i],"-date")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      date = string(argv[i]);
    } else if (!strcmp(argv[i],"-output")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      output_path = string(argv[i]);
    } else if (!strcmp(argv[i],"-region")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      region = string(argv[i]);
    } else if (!strcmp(argv[i],"-version")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      version = string(argv[i]);
    } else if (!strcmp(argv[i],"-start")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      start_hhmm = string(argv[i]);
    } else if (!strcmp(argv[i],"-end")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      end_hhmm = string(argv[i]);
    } else if (!strcmp(argv[i],"-summ_start")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      summ_start_hhmm = string(argv[i]);
    } else if (!strcmp(argv[i],"-summ_end")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      summ_end_hhmm = string(argv[i]);
    } else if (!strcmp(argv[i],"-compress")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      compression_level = atoi(argv[i]);
      compress_files = true;
    } else if (!strcmp(argv[i],"-create")) {
      create_binary_files = true;
    } else if (!strcmp(argv[i],"-sort")) {
      sort_binary_files = true;
    } else if (!strcmp(argv[i],"-late")) {
      late_summary = true;
    } else if (!strcmp(argv[i],"-price")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      interval_secs = atoi(argv[i]);
    } else if (!strcmp(argv[i],"-sids")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      sids = string(argv[i]);
    } else if (!strcmp(argv[i],"-summ")) {
      create_summary_files = true;
    } else if (!strcmp(argv[i],"-source")) {
      i++;
      if (!(--argsLeft))
        usage(argc,argv);
      source_name = string(argv[i]);
    } else {
      usage(argc,argv);
    }
    i++;
    argsLeft--;
  }

  if ((create_binary_files &&
      (quote_file.empty() || trade_file.empty() || date.empty() || output_path.empty()))
      || (sort_binary_files && (date.empty() || output_path.empty())) || source_name == "")
    usage(argc, argv);


  QBDF_Builder *qbdf_builder;
  if (source_name.substr(0, 5) == "MDSUB") {
    qbdf_builder = new MDSUB_QBDF_Builder(source_name, date);
  } else if (source_name == "LSE_Indicative") {
    qbdf_builder = new LSE_Indicative_QBDF_Builder(source_name, date);
  } else if (source_name == "NYSE_Imbalance") {
    qbdf_builder = new NYSE_Imbalance_QBDF_Builder(source_name, date);
  } else if (source_name == "NYSETAQ") {
    qbdf_builder = new NYSETAQ_QBDF_Builder(source_name, date);
  } else if (source_name == "ReutersRDF") {
    qbdf_builder = new ReutersRDF_QBDF_Builder(source_name, date);
  } else if (source_name.substr(0, 9) == "WombatCSV") {
    qbdf_builder = new WombatCSV_QBDF_Builder(source_name, date);
  }
  // else if (source_name == "BATS_Europe") {
  // qbdf_builder = new PITCH_QBDF_Builder(source_name);
  //}
  else if (source_name == "Default") {
    qbdf_builder = new QBDF_Builder(source_name, date);
  } else if (source_name == "QBT_ITCH41") {
    qbdf_builder = new QBT_ITCH41_QBDF_Builder(source_name, date);
  } else if (source_name == "QBT_ARCA") {
    qbdf_builder = new QBT_ARCA_QBDF_Builder(source_name, date);
  } else {
#ifdef CW_DEBUG
    printf("%s: ERROR - Source not supported (%s)\n", argv[0], source_name.c_str());
#endif
    exit(1);
  }

  if (create_binary_files) {
    if (qbdf_builder->create_bin_files(quote_file, trade_file, output_path, start_hhmm, end_hhmm)) {
#ifdef CW_DEBUG
      printf("%s: ERROR - unable to create binary files\n", argv[0]);
#endif
      exit(1);
    }
  }

  if (sort_binary_files) {
    if (qbdf_builder->sort_bin_files(output_path, start_hhmm, end_hhmm,
                                     summ_start_hhmm, summ_end_hhmm, late_summary)) {
#ifdef CW_DEBUG
      printf("%s: ERROR - unable to sort binary files\n", argv[0]);
#endif
      exit(1);
    }
  }

  if (create_summary_files) {
    if (qbdf_builder->create_summ_files(output_path, start_hhmm, end_hhmm,
                                        summ_start_hhmm, summ_end_hhmm, late_summary)) {
#ifdef CW_DEBUG
      printf("%s: ERROR - unable to create summary files\n", argv[0]);
#endif
      exit(1);
    }
  }

  if (compress_files) {
    if (qbdf_builder->compress_files(output_path, start_hhmm, end_hhmm, compression_level)) {
#ifdef CW_DEBUG
      printf("%s: ERROR - unable to compress files\n", argv[0]);
#endif
      exit(1);
    }
  }

  if (interval_secs > 0) {
    if (qbdf_builder->create_intraday_price_files(region, version, output_path, start_hhmm, end_hhmm,
                                                  interval_secs, sids)) {
#ifdef CW_DEBUG
      printf("%s: ERROR unable to create interim price files from binary files between %s and %s\n",
             argv[0], start_hhmm.c_str(), end_hhmm.c_str());
#endif
      exit(1);
    }
  }

  delete qbdf_builder;
#ifdef CW_DEBUG
  printf("Done\n");
#endif
  exit(0);
}
