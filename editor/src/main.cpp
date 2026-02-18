#include "editor.h"

#include <cstdlib>

auto main() -> int {
    Editor editor = Editor();
    auto initResult = editor.init();
    if (initResult)
        return initResult;

    editor.run();

    return EXIT_SUCCESS;
}
