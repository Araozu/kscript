import { ResLexer } from "../AnalisisLexico/ResLexer";
import { Token } from "../AnalisisLexico/Token";
import { InfoToken } from "../AnalisisLexico/InfoToken";
import { generarTextoError } from "./Parsers/utilidades";
import { Lexer } from "../AnalisisLexico/Lexer";

export class ErrorComun extends Error {
    constructor(message: string) {
        super(message);
    }
}

export class OpInvalida extends Error {
    constructor(message: string) {
        super(message);
    }
}

function extraerToken<A>(resLexer: ResLexer, msgError: string): Token {
    switch (resLexer.type) {
        case "ErrorLexer": {
            throw new ErrorComun(`${msgError} (${resLexer.razon})`);
        }
        case "EOFLexer": {
            throw new ErrorComun(`${msgError} (EOF)`);
        }
        case "TokenLexer": {
            return resLexer.token;
        }
        default:
            let _: never
            _ = resLexer;
            return _;
    }
}

// TODO: Limpiar
export const Expect = Object.freeze({
    Any: <A>(
        resLexer: ResLexer,
        msgError: string,
        fnErrorLexer?: (s: string) => Error,
        fnEOF?: (s: string) => Error
    ): [Token, number] | void => {
        const fnErrorLexer2 = fnErrorLexer ?? (s => new ErrorComun(s));
        const fnEOF2 = fnEOF ?? (s => new ErrorComun(s));

        switch (resLexer.type) {
            case "ErrorLexer": {
                throw fnErrorLexer2(`${msgError} (${resLexer.razon})`);
            }
            case "EOFLexer": {
                throw fnEOF2(`${msgError} (EOF)`);
            }
            case "TokenLexer": {
                return [resLexer.token, resLexer.indentacion];
            }
            default:
                let _: never
                _ = resLexer;
                return _;
        }
    },
    TNuevaLinea: (resLexer: ResLexer, msgError: string): InfoToken<undefined> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "TNuevaLinea": {
                return preToken.token;
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    TIdentificador: (fnObtToken: () => ResLexer, valorOpc: string | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(fnObtToken(), msgError);
        switch (preToken.type) {
            case "TComentario": {
                return Expect.TIdentificador(fnObtToken, valorOpc, msgError);
            }
            case "TIdentificador": {
                if (valorOpc && preToken.token.valor === valorOpc) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    TTexto: (fnObtToken: () => ResLexer, valorOpc: string | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(fnObtToken(), msgError);
        switch (preToken.type) {
            case "TComentario": {
                return Expect.TIdentificador(fnObtToken, valorOpc, msgError);
            }
            case "TTexto": {
                if (valorOpc && preToken.token.valor === valorOpc) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    PC_LET: (resLexer: ResLexer, valorOpc: string | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_LET": {
                if (valorOpc && preToken.token.valor === valorOpc) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    PC_CONST: (resLexer: ResLexer, valorOpc: string | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_CONST": {
                if (valorOpc && preToken.token.valor === valorOpc) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    PC_DO: (resLexer: ResLexer, msgError: string, lexer: Lexer): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_DO": {
                return preToken.token
            }
            case "TNuevaLinea": {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + " No se esperaba una nueva linea." + "\n" + msgErrorF);
            }
            default: {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + "\n" + msgErrorF);
            }
        }
    },
    PC_ELIF: (resLexer: ResLexer, msgError: string, lexer: Lexer): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_ELIF": {
                return preToken.token
            }
            default: {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + "\n" + msgErrorF);
            }
        }
    },
    PC_ELSE: (resLexer: ResLexer, msgError: string, lexer: Lexer): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_ELSE": {
                return preToken.token
            }
            default: {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + "\n" + msgErrorF);
            }
        }
    },
    PC_IMPORT: (resLexer: ResLexer, msgError: string, lexer: Lexer): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_IMPORT": {
                return preToken.token
            }
            default: {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + "\n" + msgErrorF);
            }
        }
    },
    PC_AS: (resLexer: ResLexer, msgError: string, lexer: Lexer): InfoToken<string> => {
        const preToken = extraerToken(resLexer, msgError);
        switch (preToken.type) {
            case "PC_AS": {
                return preToken.token
            }
            default: {
                const msgErrorF = generarTextoError<any>(lexer.entrada, preToken.token);
                throw new ErrorComun(msgError + "\n" + msgErrorF);
            }
        }
    },
    TOperador: (fnObtToken: () => ResLexer, valorOpc: string | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(fnObtToken(), msgError);
        switch (preToken.type) {
            case "TComentario": {
                return Expect.TOperador(fnObtToken, valorOpc, msgError);
            }
            case "TOperador": {
                if (valorOpc && preToken.token.valor === valorOpc) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    },
    VariosTOperador: (fnObtToken: () => ResLexer, valorOpc: string[] | undefined, msgError: string): InfoToken<string> => {
        const preToken = extraerToken(fnObtToken(), msgError);
        switch (preToken.type) {
            case "TComentario": {
                return Expect.VariosTOperador(fnObtToken, valorOpc, msgError);
            }
            case "TOperador": {
                if (valorOpc && valorOpc.find((x) => x === preToken.token.valor) !== undefined) {
                    return preToken.token;
                } else if (valorOpc) {
                    throw new ErrorComun("");
                } else {
                    return preToken.token
                }
            }
            default: {
                throw new ErrorComun(msgError);
            }
        }
    }
});
