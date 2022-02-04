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

const wchar_t *entity_manager_get_visualizer_data(entity_manager *mgr,
                                                  const entity *e) {
  if (e->_id == 0) {
    return L"<null>";
  }

  if (mgr == nullptr) {
    return L"<No entity manager>";
  }

  if (find(mgr->_entities.begin(), mgr->_entities.end(), *e) ==
      mgr->_entities.end()) {
    return L"<Invalid entity>";
  }

  // HAXX
  const cool_comp_1* c1 = mgr->_comp1s[e->_id - 1];
  if ( c1 ) {

  }
  const cool_comp_2* c2 = mgr->_comp2s[e->_id - 1];
  if ( c2 ) {

  }

  return L"TODO";
}

