#include <Util/enum.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <ASIO/Deadline_Timer.h>
#include <Util/Time.h>
#include <Interface/OrderSide.h>
#include <Util/StringUtil.h>

using namespace std;
using namespace hf_tools;
using namespace hf_core;
// //using namespace hf_core;

void map_internal_id_to_nomura_format(char * symbol, OrderSide side, const char* internal_order_id, char* outgoing_id_to_set)
{
  outgoing_id_to_set[0] = side == OrderSide::Buy ? 'B' : (side == OrderSide::SellLong ? 'S' : 'T');
  memcpy(outgoing_id_to_set + 1, symbol, 6);
  int remainder = atoi(internal_order_id);
  int cur_offset = 12;
  while(remainder > 0)
    {
      int cur_digit = remainder % 36;
      if(cur_digit > 9)
        {
          outgoing_id_to_set[cur_offset] = (cur_digit - 10) + 'A';
        }
      else
        {
          outgoing_id_to_set[cur_offset] = cur_digit + '0';
        }
      remainder /= 36;
      --cur_offset;
    }
  while(cur_offset > 6)
    {
      outgoing_id_to_set[cur_offset] = ' ';
      --cur_offset;
    }
  outgoing_id_to_set[13] = ' ';
}

void map_internal_id_to_nomura_format2(char * symbol, OrderSide side, const char* internal_order_id, char* outgoing_id_to_set)
{
  outgoing_id_to_set[0] = side == OrderSide::Buy ? 'B' : (side == OrderSide::SellLong ? 'S' : 'T');
  memcpy(outgoing_id_to_set + 1, symbol, 6);
  int remainder;
  StringUtil::get_int(internal_order_id,internal_order_id+strlen(internal_order_id),&remainder);
  int cur_offset = 12;
  while(remainder > 0)
    {
      int cur_digit = remainder % 36;
      if(cur_digit > 9)
        {
          outgoing_id_to_set[cur_offset] = (cur_digit - 10) + 'A';
        }
      else
        {
          outgoing_id_to_set[cur_offset] = cur_digit + '0';
        }
      remainder /= 36;
      --cur_offset;
    }
  while(cur_offset > 6)
    {
      outgoing_id_to_set[cur_offset] = ' ';
      --cur_offset;
    }
  outgoing_id_to_set[13] = ' ';
}


void map_internal_id_to_nomura_format3(char * symbol, OrderSide side, const char* internal_order_id, char* outgoing_id_to_set)
{
  outgoing_id_to_set[0] = side == OrderSide::Buy ? 'B' : (side == OrderSide::SellLong ? 'S' : 'T');
  memcpy(outgoing_id_to_set + 1, symbol, 6);
  int remainder;
  StringUtil::get_int(internal_order_id,internal_order_id+strlen(internal_order_id),&remainder);
  int cur_offset = 12;
  while(remainder > 0)
    {
      int cur_digit = remainder % 36;
      remainder /= 36;
      outgoing_id_to_set[cur_offset] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [ cur_digit ];
      --cur_offset;
    }
  while(cur_offset > 6)
    {
      outgoing_id_to_set[cur_offset] = ' ';
      --cur_offset;
    }
  outgoing_id_to_set[13] = ' ';
}

void map_internal_id_to_nomura_format4(char * symbol, OrderSide side, const char* internal_order_id, char* outgoing_id_to_set)
{
  outgoing_id_to_set[0] = side == OrderSide::Buy ? 'B' : (side == OrderSide::SellLong ? 'S' : 'T');
  memcpy(outgoing_id_to_set + 1, symbol, 6);
  //memset(outgoing_id_to_set + 7, ' ', 7);
  int remainder;
  StringUtil::get_int(internal_order_id,internal_order_id+strlen(internal_order_id),&remainder);
  int cur_offset = 12;
  while(remainder) {
    int cur_digit = remainder % 36;
    remainder /= 36;
    outgoing_id_to_set[cur_offset] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [ cur_digit ];
    --cur_offset;
  }
  //for( ; cur_offset > 6; --cur_offset) {
  //  outgoing_id_to_set[cur_offset] = ' ';
  //}
  if(cur_offset > 6) {
    memset(outgoing_id_to_set + 7, ' ', cur_offset - 6);
  }
  
  outgoing_id_to_set[13] = ' ';
}

void map_nomura_id_to_internal(const char* exchange_id, char* internal_order_id_to_set)
{


  int cur_offset = 12;
  int order_id_int = 0;
  int multiplier = 1;
  while(cur_offset > 6 && exchange_id[cur_offset] != ' ')
    {
      char cur_char = exchange_id[cur_offset];
      if(cur_char < 'A')
	{
	  order_id_int += (multiplier * (cur_char - '0'));
	  }
      else
	{
	  order_id_int += (multiplier * (10 + (cur_char - 'A')));
	}
      multiplier *= 36;
      --cur_offset;
    } 
  
  int remaining_internal_id = order_id_int;
  for(int offset = 8; offset >= 0; --offset)
    {
      internal_order_id_to_set[offset] = '0' + (remaining_internal_id % 10);
      remaining_internal_id /= 10;
    } 
}

void map_nomura_id_to_internal2(const char* exchange_id, char* internal_order_id_to_set)
{
  static const unsigned int type[255] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, 
    25,26,27,28,29,30,31,32,33,34,35,36,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  };
  

  int cur_offset = 12;
  int order_id_int = 0;
  int multiplier = 1;
  while(cur_offset > 6 && exchange_id[cur_offset] != ' ')
    {
      char cur_char = exchange_id[cur_offset];
      order_id_int += multiplier * type[cur_char];
      multiplier *= 36;
      --cur_offset;
    }
  int remaining_internal_id = order_id_int;
  for(int offset = 8; offset >= 0; --offset)
    {
      internal_order_id_to_set[offset] = '0' + (remaining_internal_id % 10);
      remaining_internal_id /= 10;
    } 
}


void map_nomura_id_to_internal3(const char* exchange_id, char* internal_order_id_to_set)
{
  static const uint8_t type[255] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
    0,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, 
    25,26,27,28,29,30,31,32,33,34,35,36,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  };
  
  int cur_offset = 12;
  int order_id_int = 0;
  int multiplier = 1;
  while(cur_offset > 6 && exchange_id[cur_offset] != ' ')
    {
      char cur_char = exchange_id[cur_offset];
      order_id_int += multiplier * type[cur_char];
      multiplier *= 36;
      --cur_offset;
    }
  int remaining_internal_id = order_id_int;
  for(int offset = 8; offset >= 0; --offset)
    {
      internal_order_id_to_set[offset] = '0' + (remaining_internal_id % 10);
      remaining_internal_id /= 10;
    } 
}





int main()
{
  char order_token[14];
  char* id="143000176";
  char* sym="symbol";
  OrderSide side=OrderSide::Buy;
  int num_iterations=10000000;

  map_internal_id_to_nomura_format(sym,side,id,order_token);
  cout << "Version1 token: " << order_token << endl;

  map_internal_id_to_nomura_format2(sym,side,id,order_token);
  cout << "Version2 token: " << order_token << endl;

  map_internal_id_to_nomura_format3(sym,side,id,order_token);
  cout << "Version2 token: " << order_token << endl;

  map_internal_id_to_nomura_format4(sym,side,id,order_token);
  cout << "Version2 token: " << order_token << endl;


  { 
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_internal_id_to_nomura_format(sym,side,id,order_token);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }

  { 
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_internal_id_to_nomura_format2(sym,side,id,order_token);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }

  { 
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_internal_id_to_nomura_format3(sym,side,id,order_token);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }


  {
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_internal_id_to_nomura_format4(sym,side,id,order_token);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }

  char new_id[14];
  memset(new_id, ' ', sizeof(new_id));
  map_nomura_id_to_internal(order_token,new_id);
  cout << "NEW_ID Version1 token: " << new_id << endl;

  memset(new_id, ' ', sizeof(new_id));
  map_nomura_id_to_internal2(order_token,new_id);
  cout << "NEW_ID Version1 token: " << new_id << endl;

  {
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_nomura_id_to_internal(order_token,new_id);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }

  {
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_nomura_id_to_internal2(order_token,new_id);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }


  
  {
    Time t1 = Time::current_time();
    for(size_t test = 0; test < num_iterations; ++test) {
      map_nomura_id_to_internal3(order_token,new_id);
    }
    Time t2 = Time::current_time();
    Time_Duration diff = t2 - t1;
    char time_buf[32];
    Time_Utils::print_time(time_buf, diff, Timestamp::MICRO);
    double time_per_hash = (double)diff.ticks() / num_iterations ;
    printf("%zd iterations took %s %fns per func\n", num_iterations, time_buf, time_per_hash);
  }
  



}
