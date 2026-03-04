# Refactorización del Módulo IMU - Documentación

## 📋 Resumen de la Refactorización

Se ha refactorizado el código para separar toda la lógica del IMU BNO08x en un módulo independiente (`imu.c` / `imu.h`), manteniendo `main.c` limpio y simple.

---

## 📁 Estructura de Archivos

### ✅ **Nuevos Archivos Creados**

1. **`Core/Inc/imu.h`** - Header del módulo IMU
   - Declaraciones de funciones públicas
   - Documentación de la API
   - **SEGURO para CubeMX** - Archivo externo, no se modifica

2. **`Core/Src/imu.c`** - Implementación del módulo IMU
   - Toda la lógica del IMU encapsulada
   - Variables privadas estáticas
   - Funciones privadas para procesamiento interno
   - **SEGURO para CubeMX** - Archivo externo, no se modifica

### ✅ **Archivos Modificados**

1. **`Core/Src/main.c`** - Simplificado
   - Solo inicialización de CubeMX
   - Llamadas a funciones públicas de `imu.c`
   - Todo dentro de bloques USER CODE
   - **SEGURO para CubeMX** - Solo USER CODE modificado

---

## 🔧 API Pública del Módulo IMU

### Función: `imu_init()`

```c
int imu_init(void);
```

**Descripción**: Inicializa el módulo IMU BNO08x

**Funcionalidad**:
- Inicializa el HAL SH2 para comunicación I2C
- Configura callbacks para recibir eventos del sensor
- Configura sensores Rotation Vector y Linear Acceleration

**Retorno**:
- `0` si tiene éxito
- `-1` si falla la configuración

**Uso**:
```c
if (imu_init() != 0)
{
    Error_Handler();
}
```

---

### Función: `imu_service()`

```c
void imu_service(void);
```

**Descripción**: Servicio principal del módulo IMU (llamar en el loop principal)

**Funcionalidad**:
- Lee datos del IMU vía `sh2_service()` (no bloquea)
- Procesa datos si están disponibles (en el callback)
- Envía datos por UART si hay nuevos datos
- **Refresca el watchdog SOLO cuando se envían datos por UART**

**Uso**:
```c
while (1)
{
    imu_service();
    HAL_Delay(1);
}
```

---

## 📊 Funcionalidades Encapsuladas en `imu.c`

### ✅ **Lectura de Sensores**
- Callback `sensorCallback()` para recibir eventos del sensor
- Procesamiento de datos de Linear Acceleration
- Procesamiento de datos de Rotation Vector

### ✅ **Cálculo de Orientación**
- Conversión de cuaternión a ángulos de Euler
- Cálculo de roll, pitch, heading (en grados)

### ✅ **Integración de Velocidad**
- Integración de aceleración lineal para obtener velocidad
- Intervalo de integración: 5ms (200 Hz)
- Unidades: mm/s × 1000

### ✅ **Detección de Reposo**
- Umbral: 50 mm/s²
- Contador de muestras en reposo: 5
- Reset automático de velocidad cuando está en reposo

### ✅ **Dead-Band de Velocidad**
- Umbral: 5 mm/s
- Velocidades menores al umbral se consideran cero
- Elimina drift y ruido en reposo

### ✅ **Envío por UART**
- Formato: `@imu:roll;pitch;heading;velX;velY;velZ;;\r\n`
- Timeout seguro: 100ms (no bloquea)
- Solo envía cuando hay nuevos datos

### ✅ **Gestión del Watchdog**
- **Refresca el watchdog SOLO cuando se envían datos por UART**
- Si no hay datos por más de 1 segundo, el MCU se reinicia automáticamente
- Protección contra bloqueos del sistema

---

## 🔄 Flujo de Datos

```
┌─────────────────────────────────────────┐
│  BNO08x (I2C)                           │
│  ↓                                      │
│  sh2_service() (no bloquea)            │
│  ↓                                      │
│  sensorCallback()                       │
│  ├─ Linear Acceleration                 │
│  │  ├─ Integración de velocidad        │
│  │  ├─ Detección de reposo             │
│  │  └─ Dead-band                       │
│  └─ Rotation Vector                     │
│     ├─ Cálculo roll/pitch/heading      │
│     └─ Activar print_flag               │
│  ↓                                      │
│  imu_service() detecta print_flag       │
│  ↓                                      │
│  Formatear y enviar por UART           │
│  ↓                                      │
│  HAL_IWDG_Refresh() ← WATCHDOG          │
└─────────────────────────────────────────┘
```

---

## 📝 Estructura del Código

### `main.c` - Simplificado

```c
int main(void)
{
    // Inicialización CubeMX
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART3_UART_Init();
    MX_IWDG_Init();
    
    // Inicializar módulo IMU
    if (imu_init() != 0)
    {
        Error_Handler();
    }
    
    // Loop principal
    while (1)
    {
        imu_service();  // Servicio del IMU
        HAL_Delay(1);
    }
}
```

### `imu.c` - Lógica Completa

- Variables privadas estáticas (heading, roll, pitch, velocidades, etc.)
- Funciones privadas:
  - `sensorCallback()` - Procesa datos del sensor
  - `configure_imu_sensors()` - Configura sensores
  - `split_float()` - Utilidad para formateo
- Funciones públicas:
  - `imu_init()` - Inicialización
  - `imu_service()` - Servicio principal

---

## ✅ Compatibilidad con CubeMX

### Archivos Seguros (No se modifican al regenerar):

- ✅ `Core/Inc/imu.h` - Archivo externo
- ✅ `Core/Src/imu.c` - Archivo externo
- ✅ `Core/Src/main.c` - Solo USER CODE modificado

### Verificación Post-Regeneración:

Después de regenerar código con CubeMX, verificar:

1. ✅ `main.c`: ¿Está el include `#include "imu.h"` en USER CODE BEGIN Includes?
2. ✅ `main.c`: ¿Está la llamada `imu_init()` en USER CODE BEGIN 2?
3. ✅ `main.c`: ¿Está la llamada `imu_service()` en USER CODE BEGIN 3?
4. ✅ Compilar y probar

---

## 🎯 Ventajas de la Refactorización

1. **Separación de responsabilidades**: Lógica del IMU separada de la inicialización
2. **Mantenibilidad**: Código más organizado y fácil de mantener
3. **Reutilización**: El módulo IMU puede reutilizarse en otros proyectos
4. **Testabilidad**: Más fácil de probar el módulo IMU independientemente
5. **Compatibilidad CubeMX**: Todo protegido, no se sobrescribe código generado
6. **Claridad**: `main.c` es mucho más simple y fácil de entender

---

## 📚 Documentación de Funciones

### `imu_init()`

Inicializa completamente el módulo IMU:
- Inicializa HAL SH2
- Configura callbacks
- Configura sensores (Rotation Vector a 100 Hz, Linear Acceleration a 200 Hz)

**Llamar**: Una vez al inicio, después de inicializar I2C, UART e IWDG

---

### `imu_service()`

Servicio principal que debe llamarse periódicamente:
- Lee datos del IMU (no bloquea)
- Procesa datos automáticamente (en callback)
- Envía por UART si hay nuevos datos
- Refresca watchdog solo cuando se envían datos

**Llamar**: En cada iteración del loop principal

**Comportamiento del Watchdog**:
- Se refresca solo cuando se envían datos por UART
- Si no hay datos por >1 segundo, el MCU se reinicia automáticamente
- Protege contra bloqueos del sistema

---

## 🔍 Detalles de Implementación

### Variables Privadas en `imu.c`:

```c
static volatile float heading, roll, pitch;
static volatile int print_flag = 0;
static volatile float lin_ax, lin_ay, lin_az;
static volatile int32_t velX_mm, velY_mm, velZ_mm;
static volatile uint16_t still_cnt = 0;
```

**Nota**: Todas las variables son `static`, por lo que no son accesibles desde fuera del módulo.

### Configuración de Sensores:

- **Rotation Vector**: 100 Hz (cada 10ms)
- **Linear Acceleration**: 200 Hz (cada 5ms)

### Umbrales Configurables:

```c
#define ACC_STILL_THRESHOLD   50     // mm/s²
#define STILL_COUNT_MAX       5
#define VEL_ZERO_THRESHOLD    5      // mm/s
```

---

## ✅ Checklist de Implementación

- [x] Creado `imu.h` con API pública
- [x] Creado `imu.c` con toda la lógica
- [x] Simplificado `main.c`
- [x] Watchdog refresh solo cuando se envían datos
- [x] Todo en bloques USER CODE o archivos externos
- [x] Comentarios claros en cada sección
- [x] Compatible con regeneración de CubeMX

---

## 🎉 Resultado Final

El código ahora está:
- ✅ **Bien organizado**: Lógica del IMU separada en módulo independiente
- ✅ **Simple**: `main.c` solo inicializa y llama funciones públicas
- ✅ **Mantenible**: Fácil de modificar y extender
- ✅ **Seguro**: Compatible con regeneración de CubeMX
- ✅ **Documentado**: Comentarios claros en cada sección
