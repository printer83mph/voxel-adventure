#pragma once

#include <SDL3/SDL.h>

#include <map>

typedef class Cursors {
  public:
    typedef enum Variant { DEFAULT, POINTER, MOVE, NS_RESIZE } Variant;

    Cursors();
    ~Cursors();
    auto setup_sdl_cursors() -> void;
    auto set_cursor(Variant variant) -> void;

  private:
    std::map<Variant, SDL_Cursor *> sdl_cursors;
} Cursors;
