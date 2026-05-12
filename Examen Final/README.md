# FPGA Custom Board — OrCAD PCB Design

## Descripción General

Este proyecto contiene el diseño completo de una placa PCB personalizada para FPGA, desarrollado en **OrCAD**. El diseño está basado en un **90% en la arquitectura de la Arty A7** (Digilent), una plataforma de desarrollo FPGA ampliamente utilizada con el chip Xilinx Artix-7. Para el circuito de programación y comunicación UART, se tomó como referencia adicional la **Boolean Board** de Digilent.

El objetivo de esta placa es replicar las funcionalidades esenciales de la Arty A7 en un diseño propio, permitiendo mayor control sobre el hardware y adaptando el layout a los requerimientos específicos del proyecto.

---

## Bases de Diseño

| Referencia        | Contribución al diseño                                      |
|-------------------|-------------------------------------------------------------|
| **Arty A7**       | ~90% del diseño: alimentación, FPGA, memoria, conectores    |
| **Boolean Board** | Circuito de programación (Prog) y comunicación UART         |

---

## Estructura del Proyecto

```
Examen Final/
│
├── ARTY-7/                         # Archivos fuente del diseño en OrCAD
│   ├── artix-7_ja.brd              # Archivo de layout de la PCB (Board)
│   ├── artix-7_ja.dsn              # Esquemático del diseño (Design)
│   └── arty-a7-e-ja-sch.pdf        # Esquemático exportado en PDF (referencia)
│
├── BOM/                            # Bill of Materials (Lista de Materiales)
│   └── ARTIX-7_JA.BOM.xlsx         # BOM completo con todos los componentes
│
├── Cotizacion/                     # Archivos para cotización de manufactura (JLCPCB)
│   ├── COT_ARTY.pdf                # Cotización generada
│   ├── JLCSMT_Sample_CPL1.xlsx     # Centroid / Pick and Place para ensamble SMT
│   └── Sample-BOM_JLCSMT.xlsx      # BOM en formato JLCPCB para cotización
│
└── gerbers/                        # Archivos Gerber para fabricación de la PCB
    ├── TOP.art                     # Capa superior (componentes)
    ├── BOTTOM.art                  # Capa inferior
    ├── GND.art                     # Plano de tierra principal
    ├── GND0.art                    # Plano de tierra (capa interna 0)
    ├── GND1.art                    # Plano de tierra (capa interna 1)
    ├── PWR.art                     # Plano de alimentación
    ├── 3V3.art                     # Plano de 3.3V
    ├── INTERNAL1.art               # Capa interna de señales 1
    ├── INTERNAL2.art               # Capa interna de señales 2
    ├── INTERNAL3.art               # Capa interna de señales 3
    └── desktop.ini                 # Archivo de configuración del sistema (ignorar)
```

---

## Descripción de Capas (Gerbers)

La PCB es un diseño **multicapa**, lo cual es necesario para gestionar correctamente la integridad de señales, los planos de tierra y la distribución de energía de la FPGA.

| Archivo          | Descripción                                              |
|------------------|----------------------------------------------------------|
| `TOP.art`        | Capa TOP: componentes SMD y trazado de señales superior  |
| `BOTTOM.art`     | Capa BOTTOM: señales y posibles componentes inferiores   |
| `GND.art`        | Plano de tierra dedicado                                 |
| `GND0.art`       | Plano de tierra capa interna 0                           |
| `GND1.art`       | Plano de tierra capa interna 1                           |
| `PWR.art`        | Plano de alimentación general                            |
| `3V3.art`        | Plano de distribución de 3.3V                            |
| `INTERNAL1.art`  | Señales internas — capa 1                                |
| `INTERNAL2.art`  | Señales internas — capa 2                                |
| `INTERNAL3.art`  | Señales internas — capa 3                                |

---

## Herramientas Utilizadas

- **OrCAD** — Captura de esquemático y layout de PCB
- **JLCPCB** — Servicio de fabricación y ensamble SMT
- **Microsoft Excel** — Gestión del BOM y archivos de cotización

---

## Fabricación

Los archivos en la carpeta `gerbers/` y los documentos en `Cotizacion/` están preparados para ser enviados directamente a **JLCPCB** para fabricación y ensamble SMT:

- `JLCSMT_Sample_CPL1.xlsx` → Archivo de posicionamiento de componentes (Pick & Place)
- `Sample-BOM_JLCSMT.xlsx` → BOM en el formato requerido por JLCPCB

---

## Notas

- El esquemático en PDF (`arty-a7-e-ja-sch.pdf`) sirve como referencia visual completa del diseño.
- Los archivos `.brd` y `.dsn` requieren **OrCAD PCB Designer** / **Cadence Allegro** para abrirse.
- El archivo `desktop.ini` dentro de `gerbers/` es generado automáticamente por Windows y puede ignorarse.
