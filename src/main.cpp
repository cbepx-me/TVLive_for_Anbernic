#include <cstdio>
#include "config.hpp"
#include "app.hpp"

int main(int, char**) {
    setvbuf(stderr, NULL, _IONBF, 0);
    tv::init_paths();

    tv::TVApp app;
    app.run();
    return 0;
}