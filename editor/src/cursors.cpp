#include "cursors.h"

Cursors::Cursors() {}
Cursors::~Cursors() {}

auto Cursors::setup_sdl_cursors() -> void {
    this->sdl_cursors[Variant::DEFAULT] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    this->sdl_cursors[Variant::POINTER] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    this->sdl_cursors[Variant::MOVE] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    this->sdl_cursors[Variant::NS_RESIZE] =
        SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
}

auto Cursors::set_cursor(Variant variant) -> void {
    SDL_SetCursor(this->sdl_cursors[variant]);
}
