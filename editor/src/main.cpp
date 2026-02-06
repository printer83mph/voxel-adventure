#include "editor.h"

#include <cstdlib>
#include <vxng/vxng-pub.h>

int main() {
    Editor editor = Editor();
    auto initResult = editor.init();
    if (initResult)
        return initResult;

    editor.loop();

    return EXIT_SUCCESS;
}
