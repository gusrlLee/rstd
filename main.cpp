#include <iostream>
#include "engine.hpp"

int main() {
    rstd::engine engine;

    engine.init();
    engine.run();

    engine.clean();

    return 0;
}