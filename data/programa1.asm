_start 1
.NumeroPalabras 22
.NombreProg TestFinalTotal

// --- 1. Pruebas de Sistema e Interrupciones (15, 16, 17, 18) ---
15000000 // 0: HAB      -> Activa interrupciones (PSW.int = 1)
16000000 // 1: DHAB     -> Desactiva interrupciones (PSW.int = 0)
17000050 // 2: TTI 50   -> Configura reloj a 50 ciclos
18000000 // 3: CHMOD 0  -> Cambia a modo USUARIO (PSW.modo = 0)

// --- 2. Pruebas de Registros de Protección (19, 21, 22, 20) ---
19000000 // 4: LOADRB   -> AC = 300 (Carga el Registro Base actual)
21000000 // 5: LOADRL   -> AC = 1999 (Carga el Registro Límite actual)
04101800 // 6: LOAD 1800-> Preparamos un nuevo valor de límite en AC
22000000 // 7: STRRL    -> RL = 1800 (Cambiamos el límite de memoria)
//04100350 // 8: LOAD 350 -> Preparamos un nuevo valor de base en AC
//20000000 // 9: STRRB    -> RB = 350 (Cambiamos la base física)

// --- 3. Pruebas de Stack Pointer y Movilidad (23, 24) ---
23000000 // 10: LOADSP  -> AC = SP actual (ej: 1699)
01100100 // 11: SUB 100  -> AC = 1599 (Bajamos la pila 100 posiciones)
24000000 // 12: STRSP   -> SP = 1599 (Teletransportamos la pila)

// --- 4. Pruebas de Pila Reubicada (25, 26) ---
04100999 // 13: LOAD 999 -> Cargamos 999 en AC
25000000 // 14: PSH      -> Guarda 999 en la nueva pila (MemFisica: 350+1599=1949)
04100000 // 15: LOAD 0   -> Limpiamos el AC para verificar el POP
26000000 // 16: POP      -> AC vuelve a ser 999. SP sube a 1600.

// --- 5. Prueba de Retorno de Subrutina (14) ---
04100321 // 17: LOAD 321 -> Preparamos dirección de retorno (línea 21 física)
25000000 // 18: PSH      -> Metemos el 321 en la pila
14000000 // 19: RETRN    -> Salta a la dirección 321 (Se salta la instr 20)
04100666 // 20: LOAD 666 -> ¡ESTA NO DEBE EJECUTARSE!

// --- 6. Prueba Final de Llamada al Sistema (13) ---
04100000 // 21: LOAD 0   -> AC = 0 para pedir salida
13000000 // 22: SVC 0    -> Fin del programa