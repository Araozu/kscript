open Lexer;
open Gramatica;
open Expect;


// ===================================
//  Expresion
// ===================================

type signatura =
    | Indefinida
    | Simple(string)
    | Array(signatura)
    | Tupla(list(signatura))
    | Funcion(signatura, signatura)

type eIdentificador = {
    signatura: signatura,
    valor: infoToken(string)
}

and eOperador = {
    signatura: signatura,
    valor: infoToken(string)
}

and eOperadorApl = {
    op: eOperador,
    izq: expresion,
    der: expresion
}

and eDeclaracion = {
    mut: bool,
    id: eIdentificador,
    valor: expresion
}

and expresion =
    | EIdentificador(eIdentificador)
    | EUnidad(infoToken(unit))
    | ENumero(infoToken(float))
    | ETexto(infoToken(string))
    | EBool(infoToken(bool))
    | EOperador(infoToken(string))
    | EOperadorApl(eOperadorApl)
    | EDeclaracion(eDeclaracion)
    | EBloque(list(expresion))


type exprRes =
    | PExito(expresion)
    | PError(string)
    | PEOF

type resParser =
    | ExitoParser(expresion)
    | ErrorParser(string)


type asociatividad =
    | Izq
    | Der


let obtInfoFunAppl = esCurry => ({
    valor: if (esCurry) {j|Ñ|j} else {j|ñ|j},
    inicio: -1,
    final: -1,
    numLinea: -1,
    posInicioLinea: -1
});


let obtSigIndentacion = (lexer: lexer, msgError, fnErrorLexer, fnEOF) => {
    let hayNuevaLinea = ref(false);
    try ({
        while (true) {
            let _ = _TNuevaLinea(lexer.lookAhead(), None, "");
            hayNuevaLinea := true;
            let _ = lexer.sigToken();
        };
        (1, true)
    }) {
    | ErrorComun(_) => {
        let (__, nuevaIndentacion) = _Any(lexer.lookAhead(), msgError, fnErrorLexer, fnEOF);
        (nuevaIndentacion, hayNuevaLinea^);
    }
    };
};


/**
 * El operador ñ representa aplicacion de funcion.
 * El operador Ñ representa aplicacion de funcion con currying.
 */
let obtInfoOp = (operador) => {
    let strEneMinuscula = {j|ñ|j};
    let strEneMayuscula = {j|Ñ|j}
    switch (operador) {
    | "," => (1, Izq)
    | "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "^=" => (2, Izq)
    | "<|" | "|>" => (3, Izq)
    | "<<" | ">>" => (4, Izq)
    | "||" => (5, Izq)
    | "&&" => (6, Izq)
    | "??" => (7, Izq)
    | "==" | "!=" | "===" | "!==" => (8, Izq)
    | "<" | "<=" | ">=" | ">" => (9, Izq)
    | "+" | "-" => (10, Izq)
    | "*" | "/" | "%" => (11, Izq)
    | "^" => (12, Der)
    | "." => (15, Izq)
    | _ => {
        if (operador == strEneMayuscula || operador == strEneMinuscula) {
            (14, Izq)
        } else {
            (13, Izq)
        }
    }
    };
};


let parseTokens = (lexer: lexer) => {

    let generarTextoError = info => {
        let largo = info.final - info.posInicioLinea;
        let substr = String.sub(lexer.entrada, info.posInicioLinea, largo);
        let espBlanco = String.make(info.inicio - info.posInicioLinea, ' ');
        let indicador = String.make(info.final - info.inicio, '~');
        let numLinea = info.numLinea;
        let strIndicadorNumLinea = {j| $numLinea | |j};
        let espacioBlancoIndicador = String.make(String.length(strIndicadorNumLinea), ' ');
        let strIndicador = {j|$espBlanco$indicador|j};
        {j|$strIndicadorNumLinea$substr\n$espacioBlancoIndicador$strIndicador\n|j};
    }

    let rec sigExprDeclaracion = nivel => {
        try {
            let esMut = ref(false);
            let token2 = lexer.sigToken();
            let preTokenId = ref(token2);

            try {
                let _ = _PC_MUT(token2, None, "");
                esMut := true;
                preTokenId := lexer.sigToken();
            } {
            | _ => ()
            };

            let infoTokenId = _TIdentificador(preTokenId^, None, "Se esperaba un identificador");
            let _ = _TOperador(lexer.sigToken(), Some("="), "Se esperaba el operador de asignación '=' luego del indentificador.");

            let (nuevoNivel, hayNuevaLinea) = obtSigIndentacion(lexer, "Se esperaba una expresion luego del signo '='.", None, None);

            if (hayNuevaLinea && nuevoNivel <= nivel) {
                raise(ErrorComun("La expresión actual está incompleta. Se esperaba una expresión indentada."));
            }

            switch (sigExpresion (nuevoNivel, hayNuevaLinea, 0, Izq)) {
            | PEOF => PError("Se esperaba una expresión luego de la asignacion.");
            | PError(err) => PError({j|Se esperaba una expresión luego de la asignación: $err|j});
            | PExito(exprFinal) =>
                PExito(EDeclaracion({
                    mut: esMut^,
                    id: {
                        signatura: Indefinida,
                        valor: infoTokenId
                    },
                    valor: exprFinal
                }))
            };

        } {
        | ErrorComun(err) => PError(err)
        };
    }

    and sigExprOperador = (exprIzq, infoOp: infoToken(string), nivel, precedencia, asociatividad) => {
        let valorOp = infoOp.valor
        let (precOp1, asocOp1) = obtInfoOp(valorOp);
        switch (sigExpresion(0, false, precOp1, asocOp1)) {
        | PEOF => PError({j|Se esperaba una expresión a la derecha del operador $valorOp|j})
        | PError(err) => PError({j|Se esperaba una expresion a la derecha del operador $valorOp :\n$err.|j});
        | PExito(exprFinal) => {
            let eOperadorRes: eOperador = { signatura: Indefinida, valor: infoOp }
            let exprOpRes = EOperadorApl({
                op: eOperadorRes,
                izq: exprIzq,
                der: exprFinal
            });

            switch (lexer.sigToken()) {
            | EOF => PExito(exprOpRes)
            | ErrorLexer(err) => PError(err)
            | Token(token, _) => {
                switch token {
                | TOperador(infoOp2) => {
                    let (precOp, asocOp) = obtInfoOp(infoOp2.valor);
                    sigExprOperador(exprOpRes, infoOp2, nivel, precOp, asocOp);
                }
                | TParenCer(_) => {
                    lexer.retroceder();
                    PExito(exprOpRes);
                }
                | TNuevaLinea(_) => {
                    PExito(exprOpRes);
                }
                | TIdentificador(_) | TNumero(_) | TTexto(_) | TBool(_) => {
                    let infoOp2 = obtInfoFunAppl(false);
                    // TODO: revisar si aqui se necesita agrupar la aplicacion dependiendo
                    //  de la precedencia.
                    let (precOpFunApl, asocOpFunApl) = obtInfoOp(infoOp2.valor);
                    lexer.retroceder();
                    sigExprOperador(exprOpRes, infoOp2, nivel, precOpFunApl, asocOpFunApl);
                }
                | TGenerico(infoGen) => {
                    let textoError = generarTextoError(infoGen);
                    PError({j|No se esperaba un genérico luego de la aplicación del operador.\n\n$textoError|j})
                }
                | TComentario(_) => {
                    PExito(exprOpRes);
                }
                | TParenAb(infoParen) => {
                    let sigExpr = sigExprParen(infoParen, nivel);
                    switch sigExpr {
                    | PError(err) => PError(err)
                    | PEOF => PError("Hay un parentesis sin cerrar.")
                    | PExito(expr) => {
                        let infoOpFunApl = obtInfoFunAppl(false);
                        PExito(EOperadorApl {
                            op: { signatura: Indefinida, valor: infoOpFunApl },
                            izq: exprOpRes,
                            der: expr
                        });
                    }
                    };
                }
                | PC_SEA(info) => {
                    let textoError = generarTextoError(info);
                    PError({j|No se esperaba la palabra clave 'sea' luego de la aplicación del operador.\n\n$textoError|j})
                }
                | PC_MUT(info) => {
                    let textoError = generarTextoError(info);
                    PError({j|No se esperaba la palabra clave 'mut' luego de la aplicación del operador.\n\n$textoError|j})
                }
                | TAgrupAb(info) => {
                    let textoError = generarTextoError(info);
                    PError({j|Este signo de agrupación aun no está soportado.\n\n$textoError|j});
                }
                | TAgrupCer(info) => {
                    let textoError = generarTextoError(info);
                    PError({j|Este signo de agrupación aun no está soportado.\n\n$textoError|j});
                }
                };
            }
            };
        }
        };
    }

    and sigExprIdentificador = (infoId, nivel, precedencia, asociatividad) => {
        let primeraExprId = EIdentificador {
            signatura: Indefinida,
            valor: infoId
        };

        switch (lexer.sigToken()) {
        | EOF => PExito(primeraExprId)
        | ErrorLexer(err) => PError(err)
        | Token (token, _) => {
            switch token {
            | TIdentificador(_) | TNumero(_) | TTexto(_) | TBool(_) => {
                lexer.retroceder();
                let (precFunApl, asocFunApl) = (14, Izq);
                if (precFunApl > precedencia) {
                    let infoOpFunApl = obtInfoFunAppl(false);
                    sigExprOperador(primeraExprId, infoOpFunApl, nivel, precFunApl, asocFunApl);
                } else if (precFunApl == precedencia && asocFunApl == Der) {
                    let infoOpFunApl = obtInfoFunAppl(false);
                    sigExprOperador(primeraExprId, infoOpFunApl, nivel, precFunApl, asocFunApl);
                } else {
                    PExito(primeraExprId);
                }
            }
            | TOperador(infoOp) => {
                let (precOp, asocOp) = obtInfoOp(infoOp.valor);
                if (precOp > precedencia) {
                    sigExprOperador(primeraExprId, infoOp, nivel, precOp, asocOp);
                } else if (precOp == precedencia && asocOp == Der) {
                    sigExprOperador(primeraExprId, infoOp, nivel, precOp, asocOp);
                } else {
                    lexer.retroceder();
                    PExito(primeraExprId);
                }
            }
            | _ => {
                lexer.retroceder();
                PExito(primeraExprId)
            }
            };
        };
        };
    }

    and sigExprParen = (infoParen, nivel) => {
        let sigToken = sigExpresion(nivel, false, 0, Izq);
        switch sigToken {
        | PError(_) => sigToken
        | PEOF => {
            let textoErr = generarTextoError(infoParen);
            let numLinea = infoParen.numLinea;
            let numColumna = infoParen.inicio - infoParen.posInicioLinea;
            PError({j|El parentesis abierto en $numLinea,$numColumna no está cerrado.\n\n$textoErr|j});
        }
        | PExito(sigToken2) => {
            let ultimoToken = lexer.sigToken();
            switch ultimoToken {
            | EOF => {
                let textoErr = generarTextoError(infoParen);
                let numLinea = infoParen.numLinea;
                let numColumna = infoParen.inicio - infoParen.posInicioLinea;
                PError({j|El parentesis abierto en $numLinea,$numColumna contiene una expresion, pero no está cerrado.\n\n$textoErr|j});
            }
            | ErrorLexer(error) => {
                let textoErr = generarTextoError(infoParen);
                let numLinea = infoParen.numLinea;
                let numColumna = infoParen.inicio - infoParen.posInicioLinea;
                PError({j|El parentesis abierto en $numLinea,$numColumna no está cerrado.\n\n$textoErr\nDebido a un error léxico: $error|j});
            }
            | Token (ultimoToken3, _) => {
                switch ultimoToken3 {
                | TParenCer(_) => PExito(sigToken2)
                | _ => PError("Se esperaba un cierre de parentesis.")
                };
            }
            };
        }
        };
    }

    and sigExpresion = (nivel, aceptarExprMismoNivel, precedencia, asociatividad) => {

        let resultado = lexer.sigToken();

        let sigExprActual = {
            switch resultado {
            | EOF => PEOF
            | ErrorLexer(err) => PError(err)
            | Token(token, _) => {
                switch token {
                | PC_SEA(_) => sigExprDeclaracion(nivel)
                | PC_MUT(infoPC) => {
                    let textoErr = generarTextoError(infoPC);
                    PError({j|No se esperaba la palabra clave 'mut' aquí.\n\n$textoErr|j});
                }
                | TComentario(_) => sigExpresion(nivel, aceptarExprMismoNivel, precedencia, asociatividad)
                | TNumero(infoNumero) => PExito(ENumero(infoNumero))
                | TTexto(infoTexto) => PExito(ETexto(infoTexto))
                | TBool(infoBool) => PExito(EBool(infoBool))
                | TIdentificador(infoId) => sigExprIdentificador(infoId, nivel, precedencia, asociatividad)
                | TParenAb(infoParen) => sigExprParen(infoParen, nivel)
                | TParenCer(infoParen) => {
                    let textoErr = generarTextoError(infoParen);
                    PError({j|No se esperaba un parentesis aquí. No hay ningún parentesis a cerrar.\n\n$textoErr|j});
                }
                | TNuevaLinea(_) => sigExpresion(nivel, aceptarExprMismoNivel, precedencia, asociatividad)
                | TAgrupAb(_) | TAgrupCer(_) => PError({j|Otros signos de agrupación aun no estan soportados.|j})
                | TGenerico(_) => PError({j|Los genericos aun no estan soportados.|j})
                | TOperador(infoOp) => {
                    let textoErr = generarTextoError(infoOp);
                    PError({j|No se puede usar un operador como expresión. Si esa es tu intención, rodea el operador en paréntesis, por ejemplo: (+)\n\n$textoErr|j});
                }
                };
            }
            };
        };

        switch (sigExprActual) {
        | PEOF => sigExprActual
        | PError(_) => sigExprActual
        | PExito(exprAct) => {
            try {
                let (sigNivelIndentacion, _) = obtSigIndentacion(lexer, "", Some(x => OpInvalida(x)), None);
                if (aceptarExprMismoNivel && sigNivelIndentacion == nivel) {
                    let sigExprTop = sigExpresion(nivel, aceptarExprMismoNivel, precedencia, asociatividad);
                    switch sigExprTop {
                    | PError(err) => PError(err)
                    | PEOF => sigExprActual
                    | PExito(expr) =>
                        PExito (switch expr {
                                | EBloque(exprs) =>
                                    EBloque([exprAct, ...exprs])
                                | _ =>
                                    EBloque([exprAct, expr])
                                })
                    };
                } else {
                    sigExprActual
                };
            } {
            | OpInvalida(err) => PError(err)
            | _ => sigExprActual
            };
        }
        };
    };


    let exprRe = sigExpresion(0, true, 0, Izq);
    switch (exprRe) {
    | PError(err) => ErrorParser(err);
    | PExito(expr) => ExitoParser(expr);
    | PEOF => ExitoParser(EBloque([]));
    };

};