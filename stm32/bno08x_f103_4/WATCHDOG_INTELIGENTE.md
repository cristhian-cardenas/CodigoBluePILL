# Watchdog Inteligente - Implementación Simplificada

## 🎯 Estrategia Implementada

El código ahora usa el watchdog IWDG de manera **inteligente y simple**:
- **Refresca el watchdog SOLO cuando se envían datos por UART**
- **Confía en el watchdog para resetear el MCU** si el sistema queda bloqueado
- **Elimina la recuperación compleja de I2C** - si hay problemas, el watchdog resetea y el sistema se reinicializa

---

## ⏱️ Cómo Funciona el Watchdog

### Configuración Actual (en `iwdg.c`):
- **Prescaler**: 64
- **Reload**: 624
- **Timeout aproximado**: ~1 segundo (con LSI a 40kHz)

### Cálculo del Timeout:
```
Timeout = Reload × (Prescaler / LSI_freq)
Timeout = 624 × (64 / 40000) ≈ 1.0 segundo
```

### Ajuste del Timeout (si es necesario):

Si necesitas un timeout diferente, edita `iwdg.c` en el bloque `/* USER CODE BEGIN IWDG_Init 1 */`:

```c
/* USER CODE BEGIN IWDG_Init 1 */
// Ejemplo: Para ~1.5 segundos de timeout
// hiwdg.Init.Reload = 937;  // 937 × (64/40000) ≈ 1.5 segundos
/* USER CODE END IWDG_Init 1 */
```

**Fórmula para calcular Reload:**
```
Reload = (Timeout_deseado_segundos × LSI_freq) / Prescaler
Reload = (1.5 × 40000) / 64 = 937
```

---

## 🔄 Flujo de Funcionamiento

### Escenario Normal (IMU Funcionando):
```
1. sh2_service() lee datos del IMU (no bloquea)
2. sensorCallback() procesa datos y activa print_flag
3. Loop principal detecta print_flag = 1
4. Se formatea y envía datos por UART
5. HAL_IWDG_Refresh(&hiwdg) ← WATCHDOG REFRESCADO
6. El MCU continúa funcionando normalmente
```

### Escenario de Falla (IMU Desconectado o Bloqueado):
```
1. sh2_service() no recibe datos (retorna rápidamente, no bloquea)
2. print_flag nunca se activa
3. No se envía nada por UART
4. HAL_IWDG_Refresh() NO se llama
5. El watchdog cuenta hacia abajo...
6. Después de ~1 segundo sin refresh → MCU SE REINICIA AUTOMÁTICAMENTE
7. El sistema se reinicializa completamente
```

### Escenario de Bloqueo del Código:
```
1. Si el código queda bloqueado en cualquier punto (ej: bucle infinito)
2. El loop principal no se ejecuta
3. No se envía nada por UART
4. HAL_IWDG_Refresh() NO se llama
5. Después de ~1 segundo → MCU SE REINICIA AUTOMÁTICAMENTE
```

---

## 📝 Comentarios en el Código

El código incluye comentarios claros explicando:

### Cuándo se Refresca el Watchdog:
```c
// ===== REFRESCAR WATCHDOG SOLO CUANDO SE ENVÍAN DATOS =====
// IMPORTANTE: El watchdog solo se refresca cuando realmente se envían datos por UART.
// Esto significa que:
// - Si el IMU funciona correctamente y envía datos regularmente, el watchdog se refresca
// - Si el IMU deja de enviar datos (por fallo I2C, desconexión, etc.), el watchdog NO se refresca
// - Si no hay datos por más de 1 segundo (tiempo configurado en IWDG), el MCU se reiniciará automáticamente
// - Si el código queda bloqueado en cualquier punto, el watchdog reseteará el MCU
HAL_IWDG_Refresh(&hiwdg);
```

### Cuándo el MCU se Reinicia:
- **Automáticamente** si no hay datos por más de 1 segundo
- **Automáticamente** si el código queda bloqueado
- **Automáticamente** si hay un fallo que impide el envío por UART

---

## ✅ Ventajas de Esta Implementación

1. **Simplicidad**: No hay lógica compleja de recuperación
2. **Confiabilidad**: El watchdog siempre resetea si hay problemas
3. **Eficiencia**: Solo se refresca cuando realmente hay actividad
4. **Robustez**: Funciona incluso si el código queda bloqueado
5. **Mantenibilidad**: Código más simple y fácil de entender

---

## ⚙️ Configuración Recomendada

### Timeout del Watchdog:
- **Mínimo**: 1 segundo (para detectar fallos rápidamente)
- **Recomendado**: 1.5-2 segundos (para evitar resets innecesarios por pequeños retrasos)
- **Máximo**: No más de 3 segundos (para mantener respuesta rápida a fallos)

### Frecuencia de Datos del IMU:
- **Rotation Vector**: 100 Hz (cada 10ms)
- **Linear Acceleration**: 200 Hz (cada 5ms)
- **Datos por UART**: ~100 Hz (cada 10ms)

Con esta frecuencia, el watchdog se refresca aproximadamente cada 10ms, muy por debajo del timeout de 1 segundo.

---

## 🔧 Ajuste del Timeout (si es necesario)

Si necesitas cambiar el timeout del watchdog, edita `Core/Src/iwdg.c`:

```c
/* USER CODE BEGIN IWDG_Init 1 */
// Cambiar el reload value para ajustar el timeout
// Ejemplo para 1.5 segundos: hiwdg.Init.Reload = 937;
// Ejemplo para 2 segundos: hiwdg.Init.Reload = 1250;
/* USER CODE END IWDG_Init 1 */
```

**Valores comunes:**
- 624 → ~1.0 segundo
- 937 → ~1.5 segundos
- 1250 → ~2.0 segundos
- 1875 → ~3.0 segundos

---

## 📊 Resumen

| Situación | Acción del Watchdog | Resultado |
|-----------|-------------------|-----------|
| IMU funcionando normalmente | Se refresca cada ~10ms | MCU continúa funcionando |
| IMU desconectado | No se refresca | MCU se reinicia después de ~1 segundo |
| Código bloqueado | No se refresca | MCU se reinicia después de ~1 segundo |
| Error I2C persistente | No se refresca | MCU se reinicia después de ~1 segundo |

---

## 🎯 Objetivos Cumplidos

✅ **Watchdog inteligente**: Solo se refresca cuando hay datos reales  
✅ **Reset automático**: Si no hay datos por >1 segundo, el MCU se reinicia  
✅ **Sin recuperación compleja**: Confía en el watchdog para resetear  
✅ **Loop no bloqueante**: `sh2_service()` no bloquea  
✅ **UART seguro**: Timeout de 100ms en lugar de `HAL_MAX_DELAY`  
✅ **Comentarios claros**: Explican cuándo se refresca y cuándo se reinicia  
