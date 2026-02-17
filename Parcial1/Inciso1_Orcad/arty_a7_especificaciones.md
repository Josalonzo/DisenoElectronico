# Especificaciones Generales del Proyecto - Arty A7

## 1. Que hace?

La Arty A7 es un kit de desarrollo basado en el FPGA Xilinx Artix-7 (XC7A100T-1CSG324C). Su funcion principal es permitir la implementacion de logica digital programable de alto rendimiento. El sistema integra memoria DDR3L de 256MB para almacenamiento temporal de datos, comunicacion Ethernet a 10/100 Mbps para conectividad de red, y un conversor analogico-digital integrado (XADC) para adquisicion de senales analogicas. La configuracion del FPGA se almacena de forma no volatil en dos memorias SPI Flash conectadas mediante interfaz QSPI. Para la interaccion con el usuario, el kit dispone de switches, botones, LEDs indicadores, LEDs RGB, un display de 7 segmentos y una pantalla OLED.

## 2. Que no hace?

El kit no cuenta con conectividad inalambrica, por lo que no es posible establecer comunicacion via WiFi o Bluetooth de forma nativa. Al tratarse de un FPGA y no de un procesador convencional, no ejecuta un sistema operativo directamente. Tampoco integra salidas de video como VGA o HDMI, ni interfaz de audio. Es importante considerar que la memoria DDR3L es volatil, lo que significa que los datos almacenados en ella se pierden al desconectar la alimentacion.

## 3. Como lo hara?

El FPGA Artix-7 constituye el nucleo central del sistema. Al momento de encender el kit, el FPGA carga automaticamente su configuracion desde las memorias SPI Flash a traves de la interfaz QSPI. El sistema opera con un oscilador externo de 100MHz y es capaz de alcanzar velocidades internas superiores a 450MHz.

Las senales de entrada y salida se canalizan a traves de 5 bancos de I/O (BANK 14, 15, 16, 34 y 35). Cada banco se conecta a su respectivo header de expansion, el cual distribuye las senales hacia los perifericos correspondientes. La memoria DDR3L de 256MB, con un bus de 16 bits operando a 667 MT/s, proporciona almacenamiento temporal de alta velocidad. La comunicacion con redes externas se realiza mediante un PHY Ethernet a 10/100 Mbps conectado a un puerto RJ45.

## 4. Con que lo hara?

El elemento central es el FPGA IC1G, un Xilinx Artix-7 XC7A100T-1CSG324C que organiza sus pines de entrada/salida en 5 bancos de I/O.

Para el almacenamiento, el sistema cuenta con una memoria DDR3L de 256MB con bus de 16 bits a 667 MT/s, conectada a traves del BANK 34. La configuracion del FPGA se resguarda en dos memorias SPI Flash: IC3 (S25FL128SAGNF100) e IC4 (S25FL128SAGMF100), ambas comunicadas mediante interfaz QSPI.

La comunicacion de red se realiza a traves de un PHY Ethernet a 10/100 Mbps con conector RJ45, conectado al BANK 15. El reloj del sistema lo proporciona el oscilador Y1 de 100MHz, conectado al BANK 14.

Los perifericos de entrada incluyen 4 switches, 4 botones y 1 boton de reset, todos conectados al BANK 16. Los perifericos de salida comprenden 4 LEDs indicadores, 4 LEDs RGB, un display de 7 segmentos de 4 digitos y una pantalla OLED, conectados al BANK 35.

La regulacion de voltaje esta a cargo del IC12 (MP8756GID), que convierte la entrada de 7-15V a 5V, y del IC17 (AP2303MPTR-G1), encargado de generar el voltaje de terminacion para la memoria DDR.

## 5. Que interfaces tendra?

El kit ofrece una interfaz JTAG para la programacion y depuracion del FPGA. La configuracion se almacena en dos memorias SPI Flash accesibles mediante QSPI. El bus de memoria DDR3L de 256MB se conecta a traves del BANK 34, mientras que la comunicacion Ethernet a 10/100 Mbps se establece por el BANK 15.

Para comunicacion serial, se dispone de un puente USB-UART integrado. El FPGA incluye un conversor analogico-digital on-chip (XADC) para la adquisicion de senales analogicas.

En cuanto a expansion, el kit cuenta con headers GPIO en cada banco de I/O, 4 conectores Pmod para modulos externos y un header compatible con Arduino/chipKIT para shields de expansion.

## 6. Como se alimentara?

El kit admite dos fuentes de alimentacion. La primera opcion es mediante el conector USB a 5V, que tambien se utiliza para la programacion JTAG y la comunicacion UART. La segunda opcion es una fuente externa de 7 a 15V DC, la cual es regulada a 5V por el IC12 (MP8756GID).

A partir del rail de 5V se generan los voltajes necesarios para el funcionamiento del sistema: VDD_3V3 a 3.3V para los pines de I/O del FPGA (VCCO), VDD_1V8 a 1.8V para los circuitos auxiliares como PLLs y JTAG (VCCAUX), VCCINT para el core interno de logica programable, y DDRVCC para la alimentacion de la memoria DDR3L, cuyo voltaje de terminacion es generado por el IC17 (AP2303MPTR-G1).
