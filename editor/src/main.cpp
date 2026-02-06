#include <SDL3/SDL.h>
#include <vxng/vxng-pub.h>

int main() {
    vxng::hello_world();
    SDL_Log("%s", "Hello, SDL!");
    return 0;
}
