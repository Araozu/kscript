import {flujo2} from "./Utils/flujos";

const str = `
const nombreCompleto =
    const nombre = "Juan"
    const apellido = "Perez"
    
    nombre + " " + apellido

const edad = 40
const sexo = "M"

console.log nombreCompleto edad sexo
`;

const resultado = flujo2(str, "test.ks")
    .toStringWithSourceMap({ file: "test.ks" });

console.log(resultado);
console.log(resultado.code);
console.log(resultado.map.toString());
process.exit(0);
