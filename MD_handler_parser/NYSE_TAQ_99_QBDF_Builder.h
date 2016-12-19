#ifndef hf_data_NYSE_TAQ_99_Hist_QBDF_Builder_H
#define hf_data_NYSE_TAQ_99_Hist_QBDF_Builder_H

#include <Simulator/Event_Player.h>
#include <Data/QBDF_Builder.h>

namespace hf_core {
  using namespace std;

struct trade_message
{
  uint32_t ttim;       // Number of seconds since midnight
  uint64_t price;      // 8 implied decimal places
  uint32_t size;
  uint32_t tseq;       // MDS sequence number.  Only applies to NYSE trades.
  uint16_t g127;       // Combinated "G", Rule 127, and stock trade indicator
  uint16_t corr;       // Correction indicator
  char     cond[4];    // Trade Condition
  char     ex;         // Exchange
} __attribute__ ((packed));

struct quote_message
{
  uint32_t qtim;       // Number of seconds since midnight
  uint64_t bid;        // 8 implied decimal places
  uint64_t ofr;        // 8 implied decimal places
  uint32_t qseq;       // MDS sequence number.  Only applies to NYSE quotes
  uint32_t bidsiz;
  uint32_t ofrsiz;
  uint16_t mode;       // Quote condition
  char     ex;         // Exchange
  char     mmid[4];       // NASDAQ market maker for NASD quotes
} __attribute__ ((packed));

struct index_message
{
  char     symbol[10];
  uint32_t date;       // Format: yyyymmdd
  uint32_t begrec;     // Start position
  uint32_t endrec;     // End position
} __attribute__ ((packed));

struct master_tab_message
{
  char     symbol[10];
  char     name[30];      // Company name
  char     cusip[12];
  char     etx[10];       // Set of exchanges on which the symbol trades
  char     its;           // Intermarket Trading System eligibility indicator
  char     icode[4];      // NYSE industry code
  char     sharesout[10]; // In thousands (includes treasury shares)
  char     uot[4];        // Number of shares in a round lot
  char     denom;         // Tick size
  char     type;          // Common, preferred, etc
  char     datef[8];      // Effective date for that record in the format yyyymmdd
} __attribute__ ((packed));

struct dividend_tab_message
{
  char     symbol[10];
  char     cusip[12];
  char     div[10];       // Amount of cash dividend if stock is ex-dev on this date.  5 implied decimals
  char     split_adj[10]; // Five implied decimals
  char     blank[2];
  char     datef[8];      // Date of stock split or dividend
} __attribute__ ((packed));

  class NYSE_TAQ_99_QBDF_Builder : public QBDF_Builder {
  public:
    NYSE_TAQ_99_QBDF_Builder(Application* app, const string& date)
    : QBDF_Builder(app, date)
    {
    }

    void handle_trade(trade_message& trade_msg, string& symbol);
    void handle_quote(quote_message& quote_msg, string& symbol);
  protected:
    char* load_bin_file(string& idx_file_path, string& bin_file_path, bool is_quote_file, Event_Player& event_player, vector<Event_SequenceRP>& files_list);
    int process_raw_file(const string& file_name);

  };
}

#endif /* hf_data_NYSE_TAQ_99_Hist_QBDF_Builder_H */
