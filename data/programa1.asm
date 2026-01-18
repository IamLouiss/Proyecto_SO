_start 1
.NumeroPalabras 20
.NombreProg TestDMA
// --- 1. Preparar Dato en Memoria ---
04112345 // 00: LOAD 12345 -> Cargamos un valor en AC
24000000 // 01: STRSP      -> (Truco: usaremos SP para apuntar a memoria 500)
04100500 // 02: LOAD 500
24000000 // 03: STRSP      -> SP = 500
04112345 // 04: LOAD 12345 -> Dato a guardar
25000000 // 05: PSH        -> Guardamos 12345 en Mem[500]

// --- 2. Configurar DMA para ESCRIBIR (RAM -> DISCO) ---
28000000 // 06: SDMAP 0    -> Pista 0
29000000 // 07: SDMAC 0    -> Cilindro 0
30000000 // 08: SDMAS 0    -> Sector 0
31000001 // 09: SDMAIO 1   -> 1 = ESCRITURA (RAM hacia Disco)
32000800 // 10: SDMAM 800  -> Dirección de RAM donde está el dato (500)

// --- 3. Encender DMA ---
15000000 // 11: HAB        -> Habilitamos interrupciones (Vital)
33000000 // 12: SDMAON     -> ¡Fuego! (Arranca el hilo DMA)

// --- 4. Bucle de Espera (Mientras el DMA trabaja en segundo plano) ---
// Aquí la CPU se queda dando vueltas.
// Deberías ver logs de "Instrucción ejecutada" MIENTRAS el DMA trabaja.
04100000 // 13: LOAD 0
00100001 // 14: SUM 1      -> Contamos ovejas...
27000013 // 15: J 13       -> Volver a instrucción 13 (Bucle Infinito)

// (El programa nunca llega aquí, la interrupción del DMA o del Reloj lo detendrá en log)
13000000 // 16: SVC 0