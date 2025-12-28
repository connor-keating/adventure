#pragma once

// Your application should define input_state map[KEY_COUNT];


enum input_key
{
  KEY_UNKNOWN,
  KEY_SELECT,
  KEY_ESCAPE,
  KEY_COUNT
};

enum input_state
{
  INPUT_UP,
  INPUT_HELD,
  INPUT_DOWN,
  INPUT_RELEASED
};
