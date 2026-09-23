#include "ui/app.hpp"
#include <iostream>

int main() {
    try {
        ui::App app;
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown error\n";
        return 1;
    }
}