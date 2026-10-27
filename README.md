# AGH Space Systems – zadania rekrutacyjne 2026 (Elektronika)

Rozwiązane zadania:

| Zadanie | Co to jest | Gdzie |
|---|---|---|
| **1.1 Hardware** | Moduł przetwornicy 24 V → 5 V / 3 A na AP63300, płytka 30 × 30 mm, 4 × M2 | `hardware/` |
| **1.2 Hardware** | Projekt systemu: diagram i notatka dla grzałek, pompy, belek tensometrycznych, termopar, serw i paska WS2812B (zasilanie 24 V + CAN FD) | `hardware/system_1_2/` |

## Hardware (zadanie 1.1)

- Opis, schemat, rysunki płytki i lista części z numerami MPN: [`hardware/Zadanie_1_1_opis.pdf`](hardware/Zadanie_1_1_opis.pdf)
- Projekt KiCad (wersja 10.0.7): otwórz `hardware/ap63300_buck/ap63300_buck.kicad_pro`.
  Biblioteki (symbol AP63300 i footprint cewki Bourns SRP5030HMT) leżą w tym samym folderze projektu, więc nic nie trzeba doinstalowywać.
- Pliki PDF ze schematu wyeksportowane z KiCada: `hardware/ap63300_buck/export/`.

## Hardware (zadanie 1.2)

- Diagram systemu (strona 1, format A3) i notatka z założeniami, bilansem prądów, doborem układów i złączy oraz uwagami do projektu PCB (strony 2–5):
  [`hardware/system_1_2/Zadanie_1_2_diagram_i_notatka.pdf`](hardware/system_1_2/Zadanie_1_2_diagram_i_notatka.pdf)
- Noty katalogowe wybranych układów i serii złączy: [`hardware/system_1_2/datasheets/`](hardware/system_1_2/datasheets/) (lista plików i źródeł w `README.md` w tym folderze).
