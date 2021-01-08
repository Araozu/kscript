import { Lexer } from "../../AnalisisLexico/Lexer";
import { Asociatividad } from "../Asociatividad";
import { ExprRes, PError, PExito } from "../ExprRes";
import { InfoToken } from "../../AnalisisLexico/InfoToken";
import { ErrorComun, Expect } from "../Expect";
import { TIdentificador } from "../../AnalisisLexico/Token2/TIdentificador";
import { EIdentificador } from "../Expresion/EIdentificador";
import { SignIndefinida } from "../Signatura";
import { EDeclaracionFuncion } from "../Expresion/EDeclaracionFuncion";

export function getSigExprFuncion(
    lexer: Lexer,
    sigExpresion: (
        nivel: number,
        nivelPadre: number,
        precedencia: number,
        asociatividad: Asociatividad
    ) => ExprRes,
    sigExpresionBloque: (
        nivel: number,
        esExpresion: boolean
    ) => ExprRes
) {

    function obtenerParametros(): TIdentificador[] {
        const tokenLookAhead = lexer.lookAhead();
        if (tokenLookAhead.type === "ErrorLexer" || tokenLookAhead.type === "EOFLexer") {
            throw new ErrorComun("Se esperaban parámetros luego de la palabra clave 'fun'");
        }

        // La función no tiene parámetros
        // TODO: pattern matching :c
        if (tokenLookAhead.token.type === "TUndefined") {
            lexer.sigToken();
            return [];
        } else if (tokenLookAhead.token.type === "TIdentificador") {
            const arrRetorno: TIdentificador[] = [];
            const t = tokenLookAhead.token;
            arrRetorno.push(t);
            lexer.sigToken();

            while (true) {
                const tokenRaw = lexer.lookAhead();
                if (tokenRaw.type === "ErrorLexer" || tokenRaw.type === "EOFLexer") {
                    throw new ErrorComun("Se esperaban parámetros luego de la palabra clave 'fun'");
                }

                if (tokenRaw.token.type === "TIdentificador") {
                    arrRetorno.push(tokenRaw.token);
                    lexer.sigToken();
                } else if (tokenRaw.token.type === "TOperador" && tokenRaw.token.token.valor === "=") {
                    break;
                } else {
                    throw new ErrorComun("Uno de los parámetros provistos a la función son inválidos.");
                }
            }

            return arrRetorno;
        } else {
            // TODO: Pattern matching :c
            throw new ErrorComun("El primer parámetro provisto a la función es inválido.");
        }

    }

    /**
     * Parsea una expresión de declaración de función (fun f x y = ...)
     * @param tokenFun El token 'fun'
     * @param indentacionNuevaLinea La cantidad de indentacion
     */
    function sigExprFuncion(tokenFun: InfoToken<string>, indentacionNuevaLinea: number): ExprRes {
        try {

            const infoIdentificadorFun = Expect.TIdentificador(
                lexer.sigToken.bind(lexer),
                undefined,
                "Se esperaba un identificador"
            );

            const parametros = obtenerParametros().map((x) =>
                new EIdentificador(new SignIndefinida(), x.token)
            );

            /* Robado de sigExprDeclaracion */

            Expect.TOperador(
                lexer.sigToken.bind(lexer),
                "=",
                "Se esperaba el operador de asignación '=' luego del indentificador."
            );

            const [_, nuevoNivel1, hayNuevaLinea, fnEstablecer] = lexer.lookAheadSignificativo(false);

            if (hayNuevaLinea && nuevoNivel1 <= indentacionNuevaLinea) {
                throw new ErrorComun(`La expresión actual está incompleta. Se esperaba una expresión indentada.`);
            }

            if (hayNuevaLinea) {
                fnEstablecer();
            }

            // Enviar el mayor entre el nivel del token y el nivel heredado
            const nuevoNivel = Math.max(nuevoNivel1, indentacionNuevaLinea);

            // Obtener expresion que representa el valor de la declaracion
            const sigExpr = hayNuevaLinea ?
                sigExpresionBloque(nuevoNivel, true) :
                sigExpresion(
                    nuevoNivel,
                    hayNuevaLinea ? nuevoNivel : indentacionNuevaLinea,
                    0,
                    Asociatividad.Izq
                );

            // Casos de error de la expresión inicializadora :D
            if (sigExpr.type === "PEOF" || sigExpr.type === "PReturn") {
                return new PError("Se esperaba una expresión luego de la asignación");
            } else if (sigExpr.type === "PErrorLexer") {
                return sigExpr;
            } else if (sigExpr.type === "PError") {
                return new PError(`Se esperaba una expresión luego de la asignación: ${sigExpr.err}`);
            }

            const exprFinal = sigExpr.expr;

            /* Fin robo de sigExprDeclaracion */

            const exprDeclaracionFuncion = new EDeclaracionFuncion(
                new EIdentificador(new SignIndefinida(), infoIdentificadorFun),
                parametros,
                exprFinal,
                infoIdentificadorFun.inicio,
                infoIdentificadorFun.numLinea,
                infoIdentificadorFun.posInicioLinea
            );

            return new PExito(exprDeclaracionFuncion);
        } catch (e) {
            if (e instanceof ErrorComun) {
                return new PError(e.message);
            } else {
                throw e;
            }
        }
    }

    return sigExprFuncion;
}
