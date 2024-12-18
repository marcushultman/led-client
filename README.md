# LED client

A C++ client binary that runs on Raspberry Pi. Contains different entry points for different hardware projects. Uses HTTP abstraction over Asio to communicate with a cloud-based state machine service, and SPI to render displayable stuff.

## Example: SpotiLED

LED panel using 19 + 16 * 23 APA102-based LEDs (Adafruit Neopixels). Entrypoint in [src/spotiled/main.cpp](./src/spotiled/main.cpp)

![SpotiLED](./docs/spotiled_anim.gif)