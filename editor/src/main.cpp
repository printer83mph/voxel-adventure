#include "editor.h"

#include <cstdlib>

#include <vxng/vxng.h>

auto main() -> int {
    Editor editor = Editor();
    auto initResult = editor.init();
    if (initResult)
        return initResult;

    editor.loop();

    return EXIT_SUCCESS;
}
