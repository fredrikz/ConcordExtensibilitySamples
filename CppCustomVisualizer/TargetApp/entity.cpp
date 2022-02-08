#include "entity.h"
#include <algorithm>
#include <vector>

using namespace std;

struct entity_manager {
  vector<entity> _entities;
  vector<const cool_comp_1 *> _comp1s;
  vector<const cool_comp_2 *> _comp2s;
};

entity_manager* entity_manager_create() { return new entity_manager; }
entity entity_manager_create_entity( entity_manager* mgr, const cool_comp_1* c1, const cool_comp_2* c2 ) {
  static uint32_t running_id = 1;
  entity ret{ running_id++, 1, 0 };
  mgr->_entities.push_back( ret );
  mgr->_comp1s.push_back( c1 );
  mgr->_comp2s.push_back( c2 );
  return ret;
}

static char visualizer_buffer[16 * 1024];
const char* entity_manager_get_visualizer_data(entity_manager *mgr,
                                                  const entity *e) {
  visualizer_buffer[0] = '\0';
  if (e->_id == 0) {
    return visualizer_buffer;
  }

  if (mgr == nullptr) {
    return visualizer_buffer;
  }

  if (find(mgr->_entities.begin(), mgr->_entities.end(), *e) ==
      mgr->_entities.end()) {
    return visualizer_buffer;
  }

  // HAXX
  uint64_t offset = 0;
  const cool_comp_1 *c1 = mgr->_comp1s[e->_id - 1];
  const cool_comp_2 *c2 = mgr->_comp2s[e->_id - 1];

  if (c1) {
    offset += snprintf(
        visualizer_buffer + offset, WC_ARR_LEN(visualizer_buffer) - offset,
        "'name'*((cool_comp_1*)0x%llx),", reinterpret_cast<uint64_t>(c1));
  }
  if (c2) {
    offset += snprintf(
        visualizer_buffer + offset, WC_ARR_LEN(visualizer_buffer) - offset,
        "'friends'*((cool_comp_2*)0x%llx),", reinterpret_cast<uint64_t>(c2));
  }

  visualizer_buffer[offset] = '\0';
  return visualizer_buffer;
}

