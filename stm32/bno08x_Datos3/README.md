## agregar include path correcto

Vamos a solucionarlo en **2 minutos**.

---

## 🧩 ❌ El error

```
fatal error: sh2_SensorValue.h: No such file or directory
```

Esto **NO es problema del archivo**, es problema del **include path** del compilador.

Tu estructura está bien, pero **GCC aún no sabe dónde buscar** los headers.

---

## 🛠️ SOLUCIÓN — agregar include path correcto

En **STM32CubeIDE**:

### 1️⃣ Click derecho al proyecto → **Properties**

### 2️⃣

**C/C++ Build → Settings → MCU GCC Compiler → Include paths**

### 3️⃣ Agregá exactamente estas dos rutas:

```
Core/Inc
Core/Inc/bno08x
```

(Usá el botón **Add...** → **Workspace** → navegás y las seleccionás)

### 4️⃣ Aplicar → OK

### 5️⃣ Clean & Build

```
Project → Clean
Project → Build
```

---

## 🧪 ¿Por qué pasa esto?

Dentro de `sh2_SensorValue.c` hay líneas como:

```c
#include "sh2_SensorValue.h"
```

GCC solo busca en:

- la carpeta del `.c`

- los include paths configurados

Como tu header está en:

```
Core/Inc/bno08x/sh2_SensorValue.h
```

**si no agregás `Core/Inc/bno08x` al include path, nunca lo encuentra.**

---

## 

## 🧩 Activar modo modular

Que CubeMX genere archivos como:

```
i2c.c / i2c.h
gpio.c / gpio.h
usart.c / usart.h
```

en lugar de meter todo dentro de `main.c`.

---

## 🛠️ Cómo activarlo (2 minutos)

1️⃣ Abrí tu archivo `.ioc`  
2️⃣ **Project Manager → Code Generator**  
3️⃣ Activá:

☑ **Generate peripheral initialization as a pair of .c/.h files**

4️⃣ Guardá el `.ioc`

CubeMX te va a preguntar:

> *Regenerate code?*

Decí **YES**

---

## 🧱 Tu proyecto quedará así:

```
Core
├── Inc
│   ├── i2c.h
│   ├── gpio.h
│   ├── usart.h
│   └── ...
└── Src
    ├── i2c.c
    ├── gpio.c
    ├── usart.c
    └── ...
```

Y `main.c` queda limpio y legible.

---

## 🧠 Ventaja inmediata para tu driver

Entonces en `sh2_hal.c` podés hacer lo correcto:

```c
#include "i2c.h"
```

en vez de depender de `main.h` y `extern`.

Es exactamente el entorno para el que fue pensada la librería SH2.

---
