¡Perfecto! Con esto podemos agregar los pines **I2C de tu proyecto** a la lista de conexiones. Según tu `i2c.c`, estás usando **I2C1** en el STM32F1 con los pines remapeados:

---

### 4️⃣ I2C1 para la IMU

- **SCL (reloj):** `PB8` → Pin SCL de la IMU

- **SDA (datos):** `PB9` → Pin SDA de la IMU

- **Configuración:**
  
  - Modo: `GPIO_MODE_AF_OD` (Open-Drain)
  
  - Velocidad: `GPIO_SPEED_FREQ_HIGH`
  
  - Pull-up: Necesario (interno o externo) para I2C

---

### 🔹 Lista completa de pines para tu proyecto STM32F1 + IMU + LED

| Función   | Pin STM32 | Puerto | Tipo / Nota      | Conexión física                       |
| --------- | --------- | ------ | ---------------- | ------------------------------------- |
| LED (LD2) | 13        | C      | Output PP        | LED + resistencia a GND               |
| INT IMU   | 10        | A      | Input IT_Falling | INT IMU → PA10, pull-up si open-drain |
| RST IMU   | 5         | B      | Output PP        | PB5 → RST IMU                         |
| I2C1 SCL  | 8         | B      | AF Open-Drain    | PB8 → SCL IMU                         |
| I2C1 SDA  | 9         | B      | AF Open-Drain    | PB9 → SDA IMU                         |

---

💡 **Tips importantes sobre I2C:**

- Conecta resistencias pull-up (por ejemplo, 4.7 kΩ) entre **SCL y VCC** y entre **SDA y VCC**, si tu IMU no las tiene integradas.

- Siempre verifica la alimentación de la IMU (VCC y GND) según su hoja de datos.

- Los pines PB8/PB9 están remapeados para I2C1, así que no se pueden usar como GPIO normales sin cambiar la configuración.

---

Si querés, puedo hacer un **diagrama completo de tu Nucleo F1 con la IMU, LED y las conexiones I2C/INT/RST**, para que veas todo listo para armar.

¿Querés que haga ese diagrama visual?
