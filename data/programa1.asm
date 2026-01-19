_start 0
.NumeroPalabras 20
.NombreProg test_full

// --- 1. PRUEBA DE INTERRUPCION DE RELOJ (Cod 3) ---
// Instruccion 17 (TTI). Ponemos el timer en 2 ciclos (muy rapido).
// Op: 17, Modo: 0, Val: 00002 -> 17000002
17000002

// Hacemos "tiempo" con sumas tontas para dejar que el timer explote
// LOAD #1 (Op 04, Modo 1, Val 1)
04100001
// SUM #1 (Op 00, Modo 1, Val 1) - Repetimos varias veces
00100001
00100001
00100001

// --- 2. PRUEBA DE SYSTEM CALL (Cod 2) ---
// Instruccion 13 (SVC). 
// Primero cargamos un valor != 0 en AC para que no sea "Salir" (SVC 0)
// LOAD #5
04100005
// SVC (Op 13, Modo 0, Val 0)
13000000

// --- 3. PRUEBA DE DMA (Cod 4) ---
// Ya probamos que funciona, pero lo encendemos de nuevo.
// Instruccion 33 (SDMAON)
33000000
// Mas sumas para esperar al disco
00100001
00100001
00100001

// --- 4. GRAN FINAL: ERROR FATAL (Cod 8 - Overflow/Error Matematico) ---
// Vamos a dividir por cero.
// LOAD #10
04100010
// DIV #0 (Op 03, Modo 1, Val 0) -> 03100000
03100000

// Si todo sale bien, el simulador debe morir aqui y no llegar a esta linea
27000000