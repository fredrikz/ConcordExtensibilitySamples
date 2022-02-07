#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <compare>
#include <vector>
#else
#include <stdint.h>
#endif

#define NULL_ENTITY \
  struct entity { ._id = 0, ._generation = 0, ._type = 0 }

#define WC_ARR_LEN( x ) ( sizeof(x) / sizeof(x[0]))

struct entity {
  uint32_t _id;
  uint16_t _generation;
  uint16_t _type;
#ifdef __cplusplus
  bool operator==(const entity &rhs) const {
    return _id == rhs._id && _generation == rhs._generation &&
           _type == rhs._type;
  }
  // auto operator<=>(const entity &) const = default;
#endif
};

struct cool_comp_1 {
  const char* _my_cool_name;
};

struct cool_comp_2 {
  uint32_t _my_cool_factor;
  std::vector<entity> _my_cool_friends;
  std::vector<uint32_t> _my_cool_friend_factors;
};

struct entity_manager;
entity_manager *entity_manager_create();
entity entity_manager_create_entity( entity_manager* mgr, const cool_comp_1* c1, const cool_comp_2* c2 );
const char* entity_manager_get_visualizer_data(entity_manager *mgr, const entity* e );
