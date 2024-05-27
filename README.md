# Proyecto CanSat con ESP32

Este proyecto consiste en la creación de un CanSat que utiliza varios sensores conectados a un ESP32, el cual actúa como un servidor web para la visualización de los datos en tiempo real. La pantalla OLED integrada muestra la dirección IP, el nivel de batería y los datos de los sensores.

## Descripción del Funcionamiento del Código

El código presentado permite al CanSat medir varios parámetros ambientales y comunicarlos de forma remota utilizando WiFi. A continuación, se detallan los componentes y su funcionamiento:

1. **Conectividad WiFi**: 
   - El ESP32 se conecta a una red WiFi especificada por el SSID y la contraseña. Una vez conectado, actúa como un servidor HTTP, permitiendo el acceso a los datos a través de cualquier navegador web en dispositivos que estén en la misma red.

2. **Calibración de Sensores**:
   - **MPU6050**: Se calibra inicialmente al mantener el dispositivo inmóvil durante 10 segundos, registrando valores base para aplicar compensaciones.
   - **DHT11**: Los valores de temperatura y humedad se ajustan mediante offsets determinados en una fase previa de calibración.
   - **BMP280**: Los valores de presión y altitud se ajustan con offsets determinados previamente.

3. **Lectura de Sensores**:
   - **DHT11**: Lee la temperatura y la humedad, aplicando los offsets respectivos.
   - **BMP280**: Lee la presión y la altitud, también ajustando con los offsets.
   - **MPU6050**: Lee los datos de aceleración y giroscopio, aplicando compensaciones de los valores calibrados.

4. **Visualización de Datos**:
   - Los datos de los sensores se muestran en la pantalla OLED y se envían a través del servidor web, donde se pueden visualizar en tiempo real junto con un historial de datos. La interfaz web proporciona botones para encender o apagar el MPU6050 y ver el historial de datos.

5. **Gestión de Energía**:
   - El código incluye la lectura del voltaje de la batería mediante un ADC del ESP32 y calcula el porcentaje de batería restante.

## Pines de Conexión

### ESP32 Pines
- **DHT11**: 
  - Pin de datos: GPIO 18
- **BMP280**:
  - SDA: GPIO 21
  - SCL: GPIO 22
- **MPU6050**:
  - SDA: GPIO 21
  - SCL: GPIO 22
- **OLED Display**:
  - SDA: GPIO 21
  - SCL: GPIO 22
- **ADC Pin**: 
  - GPIO 36 (ADC0) para lectura de voltaje de batería

## Requisitos

- **Hardware**:
  - ESP32
  - Sensor de temperatura y humedad DHT11
  - Sensor de presión y altitud BMP280
  - Sensor de giroscopio y acelerómetro MPU6050
  - Pantalla OLED (SSD1306)
  - Resistencias de 30k ohm y 10k ohm para el divisor de voltaje
  - Conexiones y cables necesarios

- **Software**:
  - Arduino IDE
  - Bibliotecas:
    - Wire.h
    - MPU6050.h
    - WiFi.h
    - WebServer.h
    - Adafruit_SSD1306.h
    - Adafruit_GFX.h
    - DHT.h
    - Adafruit_BMP280.h

## Instalación y Configuración

1. **Conectar los Sensores al ESP32** siguiendo la tabla de pines de conexión.
2. **Abrir el código en el Arduino IDE.**
3. **Modificar los detalles de WiFi** en las siguientes líneas del código:
    ```cpp
    const char* ssid = "Santiago's A52";
    const char* password = "12345678";
    ```
4. **Subir el código al ESP32** mediante el Arduino IDE.
5. **Abrir el Monitor Serial** en el Arduino IDE para ver los mensajes de depuración y la dirección IP asignada al ESP32.
6. **Acceder a la interfaz web de CanSat** a través de un navegador web utilizando la dirección IP mostrada en el Monitor Serial o en la pantalla OLED.

## Uso

1. **Encender el CanSat** presionando el interruptor de encendido.
2. **Esperar a que el giroscopio se calibre** (aproximadamente 10 segundos). 
3. **Conectar a la red WiFi** especificada y acceder a la dirección IP mostrada en la pantalla OLED.
4. **Visualizar y controlar los datos en tiempo real** a través de la interfaz web.

## Calibración

Los valores de calibración se ajustan dentro del código para corregir las discrepancias de los sensores. A continuación se muestran los offsets usados:

- **DHT11 (Temperatura)**: \(+0.5\,°C\)
- **DHT11 (Humedad)**: \(+2.0\,\%\)
- **BMP280 (Presión)**: \(+1.25\,hPa\)
- **BMP280 (Altitud)**: \(+0.2\,m\)
- **MPU6050 (Giroscopio)**: \(+-\) Se calibran en tiempo real

## Contribuciones

Las contribuciones al proyecto son bienvenidas. Por favor, siéntanse libres de hacer un fork del repositorio, agregar sus mejoras y enviar un pull request.
