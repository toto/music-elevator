# Floor pressure capture — 2026-09-26

The eight raw `floor*.txt` files are the Serial Monitor captures supplied for floors -1 through 6. Floor -2 is another side of floor -1 at the same height, so pressure cannot distinguish it and it has no separate table entry.

Each capture starts with readings from the previous floor and includes travel. To avoid averaging the transition, the firmware table uses the **median of valid `SAMPLE` rows in the final 15 seconds** of each file. There are 17 rows in every selected window. One malformed prefixed row in each raw file was ignored; sample text was preserved and only line endings were normalized.

| Floor | Median pressure (hPa) | Final-window range (hPa) | Track |
| ---: | ---: | ---: | --- |
| -1 | 1019.07 | 1019.06–1019.08 | `/mp3/0002.mp3` |
| 0 | 1018.79 | 1018.79–1018.80 | `/mp3/0003.mp3` |
| 1 | 1018.27 | 1018.26–1018.28 | `/mp3/0004.mp3` |
| 2 | 1017.92 | 1017.92–1017.92 | `/mp3/0005.mp3` |
| 3 | 1017.59 | 1017.58–1017.59 | `/mp3/0006.mp3` |
| 4 | 1017.21 | 1017.21–1017.22 | `/mp3/0007.mp3` |
| 5 | 1016.83 | 1016.82–1016.84 | `/mp3/0008.mp3` |
| 6 | 1016.49 | 1016.48–1016.49 | `/mp3/0009.mp3` |

Adjacent medians are 0.28–0.52 hPa apart. The smallest gap is between floors -1 and 0. The firmware still chooses the nearest configured pressure within its existing 0.15 hPa tolerance; all 136 selected settled rows classify as their labelled floor with that rule.

These are absolute pressure values from one roughly ten-minute trip. Weather or building pressure changes can shift them, so repeat the trip later and check the serial `Floor` decisions before relying on the mapping long term. Unmatched readings play `/mp3/0099.mp3`.
