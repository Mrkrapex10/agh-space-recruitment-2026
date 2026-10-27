# AGH Space Systems – zadania rekrutacyjne 2026 (Elektronika)

Rozwiązane zadania:

| Zadanie | Co to jest | Gdzie |
|---|---|---|
| **1.1 Hardware** | Moduł przetwornicy 24 V → 5 V / 3 A na AP63300, płytka 30 × 30 mm, 4 × M2 | `hardware/` |
| **2.1 + 2.2 Firmware** | STM32G474RET3: sterowanie 50 × WS2812B (TIM + DMA) oraz biblioteka TLV z obsługą ramek `msg.h` | `firmware/` |

## Hardware (zadanie 1.1)

- Opis, schemat, rysunki płytki i lista części z numerami MPN: [`hardware/Zadanie_1_1_opis.pdf`](hardware/Zadanie_1_1_opis.pdf)
- Projekt KiCad (wersja 10.0.7): otwórz `hardware/ap63300_buck/ap63300_buck.kicad_pro`.
  Biblioteki (symbol AP63300 i footprint cewki Bourns SRP5030HMT) leżą w tym samym folderze projektu, więc nic nie trzeba doinstalowywać.
- Pliki PDF ze schematu wyeksportowane z KiCada: `hardware/ap63300_buck/export/`.

