#include <Data/Wombat_Util.h>

namespace hf_core {

  uint64_t Wombat_Util::get_unique_order_id(Product::product_id_t product_id, uint64_t participant_id,
                                            uint64_t iprice)
  {
    order_id_hash_key oihk(product_id, participant_id, iprice);
    order_id_hash_t::iterator i = m_order_id_hash.find(oihk);
    if (i == m_order_id_hash.end()) {
      uint64_t order_id = ++m_order_id;
      m_order_id_hash[oihk] = order_id;
      return order_id;
    } else {
      return i->second;
    }
  }

}
