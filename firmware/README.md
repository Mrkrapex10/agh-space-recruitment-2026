# AGH Space Systems – zadania Firmware (STM32G474RET3, HAL, CMake)

Zadanie 2.1 (WS2812B) + 2.2 (biblioteka TLV + obsługa `msg.h`).

Projekt jest **kompletny i samodzielny** (HAL, CMSIS, startup i skrypt linkera są w repozytorium),
więc do zbudowania nie potrzeba STM32CubeIDE ani STM32CubeMX. Układ plików wzorowany jest na projekcie generowanym
przez CubeMX (`Core/`, `Drivers/`, `CMakeLists.txt`, `CMakePresets.json`).

## 1. Wymagania (darmowe, bez konta)

| Narzędzie | Skąd |
|---|---|
| **Arm GNU Toolchain** (`arm-none-eabi-gcc`) | https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads – wariant *AArch32 bare-metal target (arm-none-eabi)*: Windows `.exe` (mingw-w64-x86_64) lub macOS `.pkg` (Apple silicon) |
| **CMake** ≥ 3.21 | https://cmake.org/download (instalator Windows: zaznacz *Add CMake to the PATH*) lub `brew install cmake` |
| **Ninja** | https://github.com/ninja-build/ninja/releases (Windows: rozpakuj `ninja.exe` do folderu z PATH) lub `brew install ninja` |

Po instalacji sprawdź w NOWYM terminalu, że działają: `arm-none-eabi-gcc --version`, `cmake --version`, `ninja --version`.
Jeśli `arm-none-eabi-gcc` nie jest znaleziony, dodaj folder `bin` toolchaina do PATH
(macOS: `/Applications/ArmGNUToolchain/<wersja>/arm-none-eabi/bin`).
Nie umieszczaj projektu w ścieżce ze spacjami.

## 2. Kompilacja

W katalogu głównym projektu:

```
cmake --preset Debug
cmake --build --preset Debug
```

(`Release` zamiast `Debug` = kompilacja z `-Os`.) Bez presetów: `cmake -B build -G Ninja && cmake --build build`.

Wynik w `build/Debug/`: `agh_firmware.elf`, `agh_firmware.hex`, `agh_firmware.bin`
(ok. 19 KB Flash, ok. 3 KB RAM). Ostrzeżenia linkera `_close/_lseek/_read/_write is not implemented` są normalne –
to stuby newlib, których program nie używa.

Wgranie (opcjonalnie, jeśli masz płytkę): NUCLEO-G474RE widoczna jako dysk USB – skopiuj na nią `agh_firmware.bin`,
albo użyj STM32CubeProgrammer.

## 3. Struktura

```
CMakeLists.txt, CMakePresets.json, cmake/gcc-arm-none-eabi.cmake
STM32G474RETx_FLASH.ld, startup/startup_stm32g474xx.s
Core/        main.c (zegar 170 MHz), gpio, dma, tim (TIM1+DMA), usart (LPUART1), przerwania
Drivers/     STM32G4xx HAL + CMSIS (kod ST, licencja BSD-3 / Apache-2.0 – pliki LICENSE w środku)
lib/tlv/     BIBLIOTEKA TLV (bez zależności od HAL, wielokrotnego użytku)
App/         msg.h (z zadania), msg_handler, uart_link, ws2812, led_effects, app, app_config.h
tests/       testy jednostkowe uruchamiane na PC (gcc): sh tests/run_tests.sh
```

## 4. Sprzęt i ustawienia (NUCLEO-G474RE)

- Zegar: HSI 16 MHz → PLL → **170 MHz** (tryb boost).
- **WS2812B**: `PA8` (TIM1_CH1, Arduino D7) → DIN, 5 V → VDD, wspólna masa. WS2812B zasilane 5 V wymaga na DIN ok. ≥3.5 V,
  a MCU daje 3.3 V – dla jednej diody zwykle działa, dla dłuższych pasków dodaj bufor 74HCT/74AHCT.
- **UART**: `LPUART1` (`PA2`/`PA3`), 115200 8N1 – wirtualny port COM ST-LINK na Nucleo.
  Dla innej płytki zmień uchwyt/piny w `app_config.h` i `Core/Src/usart.c`.
- **Przycisk**: `PC13` (B1). Polaryzację ustawia `APP_BTN_ACTIVE_STATE` w `app_config.h`.
- Wszystkie parametry zadania 2.1 (T = 3 s, jasność, liczba mrugnięć, liczba LED) są w `App/Inc/app_config.h`.

## 5. Jak to spełnia zadania

**2.1** – `T = 3 s`: **R** prostokąt 50 % (1.5 s świeci / 1.5 s zgaszona), **G** piła (liniowo 0 → max przez 3 s, skok do 0),
**B** 3 mrugnięcia po wciśnięciu przycisku (200 ms ON / 200 ms OFF, nieblokująco, debouncing 30 ms).
Diodą steruje timer z DMA (800 kHz, jeden okres timera = jeden bit), więc procesor nie jest zajęty wysyłaniem.

**2.2** – biblioteka `lib/tlv` ma interfejsy (wskaźniki na funkcje): `write`, `get_tick_ms`, `on_frame`, `on_error`.
Żeby użyć jej w innym projekcie wystarczy skopiować `lib/tlv/` i podać te funkcje.
Przetwarzanie ramek z `msg.h` jest w `App/Src/msg_handler.c`.

## 6. ZAŁOŻENIA (treść zadania nie precyzuje – sprawdź i ewentualnie zmień)

1. **ACK**: na każdą poprawną komendę (`PING`, `MATH`) urządzenie odsyła `ACK{id, OK}`; na błędną (zła długość, nieznany typ,
   kilka flag operacji) `ACK{id, ERROR}`. Odpowiedzi (`ACK`, `PONG`, `MATH_RESP`) są ignorowane, żeby dwa urządzenia nie ACK-owały sobie ACK-ów w nieskończoność.
2. **PING**: `ACK` od razu, `PONG` dopiero po `pong_delay_ms` (nieblokująco, do 4 oczekujących naraz).
3. **MATH**: dokładnie jedna flaga operacji, bity zarezerwowane = 0. Wynik liczony na `int32`; poza zakresem `int16` ustawiana jest flaga
   `overflow`, a `result` to wartość obcięta do 16 bitów. Dzielenie przez 0: flaga `divide_by_zero`, `result = 0`. Dzielenie całkowite jak w C.
4. Pola wielobajtowe są **little-endian** (natywnie na ARM).
5. Ramka dłuższa niż `TLV_MAX_PAYLOAD_SIZE`: nadawca dostaje `ACK{type, ERROR}`, a nadmiarowe bajty są pomijane (strumień się nie rozsynchronizuje).
6. Protokół nie ma znacznika startu ani CRC, więc przerwa >100 ms w środku ramki odrzuca niedokończoną ramkę.

## 7. Testy

- PC (bez sprzętu): `sh tests/run_tests.sh` → `ALL TESTS PASSED` (protokół, PING/PONG, MATH, błędy, resynchronizacja, przebiegi LED).
- Cały projekt jest budowany `arm-none-eabi-gcc` do pliku `.elf`.
- **Kod nie był uruchamiany na prawdziwym STM32** (zadanie wymaga tylko, żeby się kompilował).

Test na płytce – terminal szeregowy z trybem HEX (np. HTerm), 115200 8N1:

| Wysyłasz (hex) | Odpowiedź (hex) |
|---|---|
| `02 02 E8 03` (PING 1 s) | `01 02 02 00`, po 1 s: `03 01 00` |
| `10 05 01 02 00 03 00` (2+3) | `01 02 10 00` `11 03 00 05 00` |
| `10 05 08 05 00 00 00` (5/0) | `01 02 10 00` `11 03 02 00 00` |
| `55 00` (nieznany typ) | `01 02 55 01` |
