// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// TargetApp.cpp : Example application which will be debuggeed

#include <iostream>
#include <windows.h>
#include "entity.h"


class MyClass
{
    const entity m_fileTime;
    const int m_anotherField;

public:
    MyClass(const entity& ft, int anotherField) :
        m_fileTime(ft),
        m_anotherField(anotherField)
    {
    }
};

struct almost_entity {
  uint32_t _id;
  uint16_t _generation;
  uint16_t _type;

  cool_comp_1 _cool_comp_1;
  cool_comp_2 _cool_comp_2;
};

entity_manager* s_entity_manager = nullptr;

int wmain(int argc, WCHAR *argv[]) {
  s_entity_manager = entity_manager_create();

  cool_comp_1 c1s[4] = {};
  cool_comp_2 c2s[4] = {};

  entity ents[4] = {
    entity_manager_create_entity(s_entity_manager, nullptr, nullptr),
    entity_manager_create_entity(s_entity_manager, c1s + 1, nullptr),
    entity_manager_create_entity(s_entity_manager, nullptr, c2s + 2),
    entity_manager_create_entity(s_entity_manager, c1s + 3, c2s + 3),
  };

  c1s[0]._my_cool_name = "Uncool name";
  c1s[1]._my_cool_name = "Coolio";
  c1s[2]._my_cool_name = "Very uncool name";
  c1s[3]._my_cool_name = "Cool Spot";

  c2s[2]._my_cool_friends.push_back( ents[3] );
  c2s[3]._my_cool_friends.push_back( ents[2] );
  c2s[2]._my_cool_factor = 32;
  c2s[3]._my_cool_factor = 13;
  c2s[2]._my_cool_friend_factors.push_back( 22 );
  c2s[3]._my_cool_friend_factors.push_back( 32 );

  entity bad_entity{1, 2, 3};
  entity null_entity{0, 0, 0};

  entity *pPointerTest1 = ents + 3;
  entity *pPointerTest2 = nullptr;

  almost_entity ae{};
  ae._id = ents[3]._id;
  ae._generation = ents[3]._generation;
  ae._type = ents[3]._type;
  ae._cool_comp_1 = c1s[3];
  ae._cool_comp_2 = c2s[3];

  MyClass c(ents[0], 12);


  __debugbreak(); // program will stop here. Evaluate 'creationTime' and
                  // 'pPointerTest' in the locals or watch window.
  std::cout << "Test complete\n";

  return 0;
}

