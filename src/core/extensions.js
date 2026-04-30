import { createLinearUnit } from "./units.js";

export function createExtensionRegistry(setup = null) {
  const registry = {
    variables: new Map(),
    functions: new Map(),
    units: [],
  };

  if (setup) {
    setup(createNumiApi(registry));
  }

  return registry;
}

function createNumiApi(registry) {
  return {
    setVariable(name, value) {
      registry.variables.set(name, Number(value));
    },
    addFunction(name, fn) {
      registry.functions.set(name.toLowerCase(), (values) => Number(fn(...values)));
    },
    addUnit(id, definition) {
      const ratio = Number(definition.ratio ?? definition.value);
      const aliases = definition.aliases ?? [id];
      const dimension = definition.dimension ?? `extension:${id}`;
      registry.units.push(createLinearUnit(id, dimension, ratio, aliases));
    },
  };
}
