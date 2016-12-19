#ifndef hf_core_Data_Wombat_Util_h
#define hf_core_Data_Wombat_Util_h

#include <Product/Product.h>
#include <ext/hash_map>
#include <boost/functional/hash.hpp>

namespace hf_core
{
  struct order_id_hash_key {
    Product::product_id_t product_id;
    uint64_t participant_id;
    uint64_t iprice;

    order_id_hash_key(Product::product_id_t prod_id, uint64_t part_id, uint64_t ipx)
    :  product_id(prod_id), participant_id(part_id), iprice(ipx) {}

    bool operator==( const hf_core::order_id_hash_key& x ) const
    {
      if (participant_id == x.participant_id && iprice == x.iprice
          && product_id == x.product_id) {
        return true;
      }
      return false;
    }
  };
};

namespace __gnu_cxx
{
  template<> struct hash< hf_core::order_id_hash_key >
  {
    std::size_t operator()( const hf_core::order_id_hash_key& x ) const
    {
      std::size_t seed = 0;
      boost::hash_combine(seed, x.product_id);
      boost::hash_combine(seed, x.participant_id);
      boost::hash_combine(seed, x.iprice);

      return seed;
    }
  };
};

namespace hf_core
{
  typedef __gnu_cxx::hash_map<order_id_hash_key,uint64_t> order_id_hash_t;

  class Wombat_Util {
    public:
      Wombat_Util() : m_order_id(0) {}

      uint64_t get_unique_order_id(Product::product_id_t product_id, uint64_t participant_id, uint64_t iprice);
    private:
      order_id_hash_t m_order_id_hash;
      uint64_t m_order_id;
  };
};




#endif /* ifndef hf_core_Data_Wombat_Util_h */
