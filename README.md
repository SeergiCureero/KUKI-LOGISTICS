# KUKI-LOGISTICS

En este repositorio subiremos el codigo de nuestro proyecto de AGV.

La estructura de comunicacion se divide en 3 partes:
- Mando Kuki: es un mando externo que se comunica con la KUKI (el AGV) por bluetooth.
- KUKI RP2040: es la placa que recibe esta comunicación, pero tambien es capaz de gestionar entradas e infromacion por si sola. Le envia instruciones basicas, digeridas, por serie a la placa MEGA 2560. Aqui se almacena el circuito y los pasos.
- Esclavo MEGA 2560: Esta placa ejecuta las instrucciones de la KUKI y gestiona informacion de nivel muy básico. Todo el codigo de gestion de motores está aqui.