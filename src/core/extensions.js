import vm from "node:vm";
import { createLinearUnit } from "./units.js";

const CURRENT_EXTENSION_API_VERSION = 1;

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

export function loadExtensionModules(modules) {
  const registry = createExtensionRegistry();
  const diagnostics = [];

  for (const module of modules) {
    diagnostics.push(...loadExtensionModule(registry, module));
  }

  return { registry, diagnostics };
}

export function loadExtensionModule(registry, module) {
  const manifestDiagnostics = validateExtensionManifest(module.manifest);
  if (manifestDiagnostics.length > 0) {
    return manifestDiagnostics;
  }

  const extensionId = module.manifest.id;

  try {
    const context = vm.createContext({
      Math,
      Number,
      String,
      numi: createNumiApi(registry),
    });
    const script = new vm.Script(String(module.source ?? ""), {
      filename: module.manifest.entry ?? `${extensionId}.js`,
    });
    script.runInContext(context, { timeout: 250 });
    return [];
  } catch (error) {
    return [{
      severity: "error",
      extensionId,
      message: normalizeRuntimeErrorMessage(error),
    }];
  }
}

export function validateExtensionManifest(manifest) {
  const extensionId = typeof manifest?.id === "string" && manifest.id.length > 0
    ? manifest.id
    : "<unknown>";
  const diagnostics = [];

  if (typeof manifest?.id !== "string" || manifest.id.length === 0) {
    diagnostics.push({
      severity: "error",
      extensionId,
      message: "Extension manifest id must be a non-empty string",
    });
  }
  if (manifest?.apiVersion !== CURRENT_EXTENSION_API_VERSION) {
    diagnostics.push({
      severity: "error",
      extensionId,
      message: `Extension apiVersion must be ${CURRENT_EXTENSION_API_VERSION}`,
    });
  }
  if (manifest?.entry !== undefined && typeof manifest.entry !== "string") {
    diagnostics.push({
      severity: "error",
      extensionId,
      message: "Extension manifest entry must be a string when provided",
    });
  }

  return diagnostics;
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

function normalizeRuntimeErrorMessage(error) {
  const message = error instanceof Error ? error.message : String(error);
  return message.replace(/^Error:\s+/u, "");
}
