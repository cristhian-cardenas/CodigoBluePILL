🧪 Habilitar `printf` con soporte para `float` en STM32CubeIDE

Por defecto, STM32CubeIDE **no incluye soporte para imprimir números `float` con `printf`**, lo que provoca el siguiente warning o error al compilar:

```
The float formatting support is not enabled
```

### 🔧 Solución

En el proyecto:

1. Click derecho sobre el proyecto

2. **Properties**

3. **C/C++ Build → Settings**

4. **Tool Settings → MCU GCC Linker → Miscellaneous**

5. En **Other flags** agregar:

```
-u _printf_float
```

6. Presionar **Apply and Close**

7. Ejecutar **Clean** y luego **Build**

### 🧪 Prueba

```c
float x = 3.1416f;
printf("Valor: %.2f\r\n", x);
```

Salida esperada:

```
Valor: 3.14
```

> ⚠️ Nota: habilitar `printf` con `float` aumenta ligeramente el tamaño del firmware, lo cual es normal.