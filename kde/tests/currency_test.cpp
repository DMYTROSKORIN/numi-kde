#include <libqalculate/qalculate.h>
#include <iostream>

int main() {
    Calculator calc;
    calc.loadGlobalDefinitions();
    
    std::string names[] = {"to", "in", "as", "from", "of", "today", "now"};
    for (const auto &name : names) {
        if (calc.getUnit(name)) std::cout << name << " is a unit" << std::endl;
        if (calc.getVariable(name)) std::cout << name << " is a variable" << std::endl;
        if (calc.getFunction(name)) std::cout << name << " is a function" << std::endl;
    }
    
    return 0;
}
