# raw/topotijdreis

Screenshots of the topographic map 1:25 000 (TOP25) of earlier years, taken from Topotijdreis
(https://www.topotijdreis.nl, Kadaster; map tiles served by Esri Nederland) on 2026-11-21, by Jan de Kort.
Each is the compare view at the deepest zoom: the old edition on the left, the edition of 1998 on the right,
both of the same piece of ground and at the same scale.

- `Map_1965.png` — edition shown for 1965, beside 1998
- `Map_1983.png` — edition shown for 1983, beside 1998
- `Map_1994.png` — edition shown for 1994, beside 1998

They are raw data: never edited. `tools/make_old_park.py` reads the buildings of the old park
(1957 to 1996) off them and writes `data/old_park.json`. The grid lines that cross in each panel are those of
RD x = 119000 and y = 575000; the 1998 panel, which shows the houses of today, gives the scale.
