# Especificaciones Generales del Proyecto - Arty A7

## 1. Que hace?

La Arty A7 es un kit de desarrollo basado en un FPGA de la familia Artix-7 de Xilinx. Su funcion principal es permitir la implementacion de logica digital programable de alto rendimiento. El sistema integra memoria DDR3L de 256MB para almacenamiento temporal de datos, comunicacion Ethernet a 10/100 Mbps para conectividad de red, y un conversor analogico-digital integrado para adquisicion de senales analogicas. La configuracion del FPGA se almacena de forma no volatil en dos memorias Flash conectadas mediante interfaz QSPI. Para la interaccion con el usuario, el kit dispone de switches, botones, LEDs indicadores, LEDs RGB, un display de 7 segmentos y una pantalla OLED.

## 2. Que no hace?

El kit no cuenta con conectividad inalambrica, por lo que no es posible establecer comunicacion via WiFi o Bluetooth de forma nativa. Al tratarse de un FPGA y no de un procesador convencional, no ejecuta un sistema operativo directamente. Tampoco integra salidas de video como VGA o HDMI, ni interfaz de audio. Es importante considerar que la memoria DDR3L es volatil, lo que significa que los datos almacenados en ella se pierden al desconectar la alimentacion.

## 3. Como lo hara?

El FPGA constituye el nucleo central del sistema. Al momento de encender el kit, el FPGA carga automaticamente su configuracion desde las memorias Flash a traves de la interfaz QSPI. El sistema opera con un oscilador externo de 100MHz y es capaz de alcanzar velocidades internas superiores a 450MHz.

Las senales de entrada y salida se canalizan a traves de 5 bancos de I/O. Cada banco se conecta a su respectivo header de expansion, el cual distribuye las senales hacia los perifericos correspondientes. La memoria DDR3L de 256MB, con un bus de 16 bits operando a 667 MT/s, proporciona almacenamiento temporal de alta velocidad. La comunicacion con redes externas se realiza mediante un transceptor Ethernet a 10/100 Mbps conectado a un puerto RJ45.

## 4. Con que lo hara?

El elemento central es un FPGA de la familia Artix-7 de Xilinx, que organiza sus pines de entrada/salida en 5 bancos de I/O.

Para el almacenamiento, el sistema cuenta con una memoria DDR3L de 256MB con bus de 16 bits a 667 MT/s. La configuracion del FPGA se resguarda en dos memorias Flash, ambas comunicadas mediante interfaz QSPI.

La comunicacion de red se realiza a traves de un transceptor Ethernet a 10/100 Mbps con conector RJ45. El reloj del sistema lo proporciona un oscilador externo de 100MHz.

Los perifericos de entrada incluyen 4 switches, 4 botones y 1 boton de reset. Los perifericos de salida comprenden 4 LEDs indicadores, 4 LEDs RGB, un display de 7 segmentos de 4 digitos y una pantalla OLED.

La regulacion de voltaje esta a cargo de un regulador principal que convierte la entrada de 7-15V a 5V, y un regulador secundario encargado de generar el voltaje de terminacion para la memoria DDR.

## 5. Que interfaces tendra?

El kit ofrece una interfaz JTAG para la programacion y depuracion del FPGA. La configuracion se almacena en dos memorias Flash accesibles mediante QSPI. El bus de memoria DDR3L de 256MB se comunica directamente con el FPGA, mientras que la comunicacion Ethernet a 10/100 Mbps se establece a traves de un transceptor dedicado.

Para comunicacion serial, se dispone de un puente USB-UART integrado. El FPGA incluye un conversor analogico-digital integrado para la adquisicion de senales analogicas.

En cuanto a expansion, el kit cuenta con headers GPIO en cada banco de I/O, 4 conectores Pmod para modulos externos y un header compatible con Arduino/chipKIT para shields de expansion.

## 6. Como se alimentara?

El kit admite dos fuentes de alimentacion. La primera opcion es mediante el conector USB a 5V, que tambien se utiliza para la programacion JTAG y la comunicacion UART. La segunda opcion es una fuente externa de 7 a 15V DC, la cual es regulada a 5V por un regulador integrado en la tarjeta.

A partir del rail de 5V se generan los voltajes necesarios para el funcionamiento del sistema: 3.3V para los pines de I/O del FPGA, 1.8V para los circuitos auxiliares como PLLs y JTAG, un rail dedicado para el core interno de logica programable, y otro rail para la alimentacion y terminacion de la memoria DDR3L.
