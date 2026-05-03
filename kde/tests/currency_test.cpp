#include <libqalculate/qalculate.h>
#include <iostream>

int main() {
    Calculator calc;
    calc.loadGlobalDefinitions();
    calc.loadExchangeRates();

    EvaluationOptions eo;
    eo.parse_options.angle_unit = ANGLE_UNIT_RADIANS;
    eo.structuring = STRUCTURING_SIMPLIFY;
    eo.parse_options.unknowns_enabled = false;

    PrintOptions po;
    po.number_fraction_format = FRACTION_DECIMAL;
    po.base = 10;
    po.max_decimals = 3;

    // Test with "A := 600 AED + 400 USD"
    std::string expr = "A := 600 AED + 400 USD";
    MathStructure result = calc.calculate(expr, eo);
    std::cout << "Result: " << calc.print(result, 2000, po) << std::endl;
    
    // Check if A is a variable
    Variable *v = calc.getVariable("A");
    if (v) std::cout << "A is a variable" << std::endl;
    else std::cout << "A is NOT a variable" << std::endl;

    return 0;
}
