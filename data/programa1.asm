_start 1
.NumeroPalabras 23
.NombreProg TestTotal
// --- 1. Pruebas de Sistema (15, 16, 17, 18) ---
15000000 // 00: HAB      -> Activa interrupciones
17000002 // 02: TTI 2   -> Configura reloj (Solo print por ahora)
// --- 2. Prueba de Seguridad CHMOD (18) ---
// Intentamos pasar a Kernel (1). Como estamos en Usuario (0), 
// la CPU debería GRITAR un error de "Violación de Privilegios" y no cambiar.
18000001 // 03: CHMOD 1  -> ¡Debe fallar o dar error de seguridad!
18000000 // 04: CHMOD 0  -> Cambia a Usuario (Seguro)
// --- 3. Pruebas de Registros de Protección (19, 21, 22) ---
19000000 // 05: LOADRB   -> AC = 300 (Base actual)
21000000 // 06: LOADRL   -> AC = 1999 (Límite actual)
04101800 // 07: LOAD 1800-> AC = 1800
22000000 // 08: STRRL    -> RL = 1800 (Achicamos la memoria del proceso)
// --- 4. Pruebas de Stack Pointer y Movilidad (23, 24) ---
23000000 // 09: LOADSP   -> AC = SP actual (aprox 1699)
01100100 // 10: RES 100  -> AC = 1599 (Bajamos el techo 100 pisos)
24000000 // 11: STRSP    -> SP = 1599 (Movemos la pila de lugar)
// --- 5. Pruebas de Pila Reubicada (25, 26) ---
04100999 // 12: LOAD 999 -> AC = 999 (Dato a guardar)
25000000 // 13: PSH      -> Guarda 999 en Stack[1599] (Física 300+1599=1899)
04100000 // 14: LOAD 0   -> AC = 0 (Borramos AC para probar)
26000000 // 15: POP      -> AC debe volver a ser 999
// --- 6. Prueba de Retorno de Subrutina (14) ---
// Queremos saltar a la instrucción 20 (LOAD 1) y saltarnos la 19 (LOAD 666)
// La instrucción 20 es la dirección relativa 20.
04100020 // 16: LOAD 20  -> Dirección de retorno (Relativa)
25000000 // 17: PSH      -> La metemos en la pila
14000000 // 18: RETRN    -> Saca el 20 y lo pone en el PC. Salta allá.
04100666 // 19: LOAD 666 -> ¡TRAMPA! NO DEBE EJECUTARSE.
// --- 7. Aterrizaje y Fin (13) ---
04100001 // 20: LOAD 1   -> Si AC=1 al final, el salto funcionó.
04100000 // 21: LOAD 0   -> AC = 0 para terminar
13000000 // 22: SVC 0    -> Fin del programa