import connected from "../../assets/city/lagos-connected-transit-v1.json";
import streetDensity from "../../assets/city/lagos-street-density-navigation-v1.json";

// Renderer packaging supplies lossless, on-demand copies of these immutable recipes.
// Other consumers keep ordinary JSON imports and the same inferred data types.
export function connectedCityData() { return connected; }
export function streetDensityCityData() { return streetDensity; }
