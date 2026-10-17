# Console Musicale Joystick

Console musicale per **Arduino UNO** che si suona con **due joystick**: ogni direzione è una nota, da Do a Do acuto. Con le diagonali e muovendo i due stick insieme si suonano gli accordi. Due buzzer passivi fanno sentire due note davvero insieme e un buzzer attivo fa da percussione. Un display LCD mostra il menu, i turni e i punteggi, e ci sono giochi da 1 a 4 giocatori.

Sketch: `console_joystick_3buzzer.ino`

---

## Componenti

Tutti i componenti vengono dallo starter kit Elegoo UNO R3 e dal kit da 37 sensori.

| Quantità | Componente |
|---|---|
| 1 | Arduino UNO R3 |
| 2 | Modulo joystick (uno per kit) |
| 1 | Chip 74HC595 |
| 8 | LED + 8 resistenze da 330 Ω |
| 2 | Buzzer passivi |
| 1 | Buzzer attivo |
| 1 | Display LCD1602 + resistenza da 220 Ω |
| 1 | Potenziometro 10 kΩ (contrasto LCD) |
| 1 | Breadboard da 830 punti |
| 9 | Cavetti femmina-maschio (per i joystick) |
| — | Cavetti per breadboard |

---

## Le note sui joystick

```
      JOYSTICK 1             JOYSTICK 2
          Do                     Sol
      Mi  +  Fa              Si   +  Do'
          Re                     La
```

Le note vicine (Do-Re, Mi-Fa, Sol-La, Si-Do') stanno in direzioni opposte. È il trucco che rende suonabili tutti gli accordi: ognuno si fa con una diagonale su uno stick e una direzione sull'altro.

### Come si suonano gli accordi

| Accordo | Note | Movimento |
|---|---|---|
| Do maggiore | Do Mi Sol | J1 su-sinistra + J2 su |
| Re minore | Re Fa La | J1 giù-destra + J2 giù |
| Mi minore | Mi Sol Si | J1 sinistra + J2 su-sinistra |
| Fa maggiore | Fa La Do | J1 su-destra + J2 giù |
| Sol maggiore | Sol Si Re | J2 su-sinistra + J1 giù |
| La minore | La Do Mi | J2 giù + J1 su-sinistra |
| Si diminuito | Si Re Fa | J2 sinistra + J1 giù-destra |

---

## Tre buzzer, tre ruoli

| Buzzer | Pin | Ruolo |
|---|---|---|
| Passivo 1 | D9 | **Voce principale.** Suona la melodia e la nota più bassa degli accordi. L'onda la genera il Timer1 del chip, per questo deve stare proprio su D9. |
| Passivo 2 | D13 | **Seconda voce.** Con due note ne suona una lei, così si sentono insieme. Con una nota sola la rinforza un'ottava sotto. Nel juke-box fa l'accompagnamento. Con 3 o 4 note alterna velocissima le più alte (arpeggio). |
| Attivo | A4 | **Percussione.** Batte il tempo nel juke-box, ticchetta negli ultimi 3 secondi per rispondere e ronza quando sbagli. |

Per riconoscerli: il buzzer attivo ha l'adesivo e il fondo chiuso, i passivi mostrano sotto il circuito verde. Se colleghi solo l'alimentazione, l'attivo suona da solo e il passivo resta muto.

---

## I giochi

All'accensione l'LCD scorre il menu: spingi lo stick nella direzione indicata.

| Direzione | Gioco | Giocatori | Descrizione |
|---|---|---|---|
| J1 su | **Simon** | 1–4 | A turno ognuno ripete la sequenza di note, che si allunga. Chi sbaglia è eliminato. Da soli si gioca per il record. |
| J1 giù | **Catena** | 2–4 | Ripeti a memoria la catena suonata dagli altri, senza aiuto, poi aggiungi una nota tua. |
| J1 sinistra | **Orecchio** | 1–4 | Senti il Do e poi una nota misteriosa a luci spente: spingi nella direzione giusta. |
| J1 destra | **Accordi** | 1–4 | L'LCD scrive un accordo: suonalo e tienilo per mezzo secondo. |
| J2 su | **Piano** | — | Suoni con tutte e due le mani. L'LCD scrive l'accordo che stai suonando. |
| J2 giù | **Juke-box** | — | Dieci melodie con accompagnamento e batteria. |
| J2 sinistra | **Test** | — | Mostra i valori dei joystick e le note riconosciute. |

- **Numero di giocatori:** joystick 1 su = 1, giù = 2, sinistra = 3, destra = 4.
- **Tornare al menu:** tieni premuto lo stick del joystick 1 per un secondo. Un clic breve viene ignorato.
- **Azzerare il record di Simon:** nel Test tieni Do e Si insieme per 3 secondi.

### Comandi del juke-box

| Joystick 1 | Azione |
|---|---|
| su | brano successivo |
| giù | più lento |
| sinistra | più veloce |
| destra | batteria sì/no |

### Brani del juke-box

Tutti di pubblico dominio.

| Brano | Autore e origine |
|---|---|
| Fra Martino | canto tradizionale francese |
| Inno alla gioia | Ludwig van Beethoven, Nona sinfonia (1824) |
| Brilla stellina | melodia francese del Settecento |
| Jingle Bells | James Lord Pierpont (1857) |
| Tanti auguri | Mildred e Patty Hill (1893) |
| Toreador | Georges Bizet, Carmen (1875) |
| Gran Vals | Francisco Tárrega (1902), il frammento usato come suoneria Nokia |
| Korobeiniki | canto popolare russo (1861), il tema di Tetris |
| Passacaglia | Georg Friedrich Händel |
| Tri martolod | melodia tradizionale bretone |

Le ultime sette arrivano dalla raccolta [hibit-dev/buzzer](https://github.com/hibit-dev/buzzer) (codice con licenza MIT, © 2022 HiBit).

Per aggiungere un brano, scrivi nello sketch un array di coppie `{frequenza in Hz, durata in sedicesimi}`. Le durate valgono 1 = sedicesimo, 2 = ottavo, 4 = quarto, 6 = quarto puntato, 8 = minima, 16 = semibreve; frequenza 0 vuol dire pausa. Poi aggiungi una riga `BRANO(...)` alla tabella `BRANI`.

---

## Collegamenti

| Componente | Collegamenti |
|---|---|
| Joystick 1 | GND → binario − · +5V → binario + · VRx → A0 · VRy → A1 · SW → D2 (tasto MENU) |
| Joystick 2 | GND → binario − · +5V → binario + · VRx → A2 · VRy → A3 · SW non collegato |
| 74HC595 | colonne 18–25, tacca a sinistra · pin 16 VCC e pin 10 MR → + · pin 13 OE e pin 8 GND → − · pin 14 DS → D10 · pin 12 latch → D11 · pin 11 clock → D12 |
| 8 LED | uscite Q0…Q7 (pin 15, 1, 2, 3, 4, 5, 6, 7) → gamba lunga di Do…Do' · gamba corta → 330 Ω → binario − |
| Buzzer passivo 1 | + → D9 (solo D9) · − → binario − |
| Buzzer passivo 2 | + → D13 · − → binario − |
| Buzzer attivo | + (gamba lunga) → A4 · − → binario − |
| LCD1602 | colonne 48–63 riga j · RS → D3 · E → D4 · D4–D7 → D5–D8 · VSS, RW, K → − · VDD → + · A → 220 Ω → + · V0 → centrale del potenziometro |
| Potenziometro 10 kΩ | colonne 53–55 · laterali a + e − · centrale → V0 dell'LCD |
| Alimentazione | 5V di Arduino → binario + in alto · GND → binario − in alto · due ponticelli collegano i binari in alto con quelli in basso |
| Pin libero | A5 |

### Riepilogo dei pin di Arduino

| Pin | Uso |
|---|---|
| D2 | SW joystick 1 (MENU) |
| D3–D8 | LCD (RS, E, D4, D5, D6, D7) |
| D9 | buzzer passivo 1 |
| D10, D11, D12 | 74HC595 (dati, latch, clock) |
| D13 | buzzer passivo 2 |
| A0, A1 | joystick 1 (X, Y) |
| A2, A3 | joystick 2 (X, Y) |
| A4 | buzzer attivo |
| A5 | libero |

### Note sulla breadboard

- I due joystick restano fuori dalla breadboard e si collegano con i cavetti femmina-maschio: 5 per il primo, 4 per il secondo.
- Il corpo dell'LCD sporge oltre il bordo e copre i binari in basso a destra: per l'alimentazione usa i binari in alto.
- Su alcune breadboard i binari + e − sono interrotti a metà. Se è così, unisci le due metà con un cavetto.

---

## Come funziona

**Joystick.** Ogni joystick contiene due potenziometri, uno orizzontale e uno verticale. Arduino legge un valore da 0 a 1023: al centro vale circa 512. Sotto 260 o sopra 764 la direzione conta come spinta. Le soglie sono abbastanza larghe da riconoscere la diagonale come due note insieme, e il codice ignora le diagonali attraversate per un attimo mentre muovi lo stick.

**LED.** Gli 8 LED sono comandati dal chip 74HC595, che con 3 soli pin pilota fino a 8 uscite.

**Due voci.** Il buzzer passivo 1 è pilotato direttamente dal Timer1 in modo CTC (frequenza = 16 MHz / (2 · 8 · (OCR1A + 1))). Il passivo 2 usa `tone()`, che lavora sul Timer2. Avendo due timer separati, le due note suonano davvero insieme.

---

## Prima accensione

1. Orienta i joystick con i pin verso di te, poi dal menu scegli **J2 sinistra: Test**. Spingi ogni stick nelle quattro direzioni e controlla sull'LCD che compaia la nota giusta.
2. Se su e giù sono scambiati, nel codice metti `true` in `INVERTI_Y` per quel joystick. Se sono scambiati sinistra e destra, in `INVERTI_X`. Se le direzioni sono ruotate di 90°, gira il joystick.
3. Se una diagonale non suona due note, abbassa un po' `SOGLIA_ALTA` e alza `SOGLIA_BASSA`.
4. Il 74HC595 va con la tacca a sinistra, a cavallo del solco centrale.
5. Gira il potenziometro finché le scritte dell'LCD sono nitide.
6. Se una voce è molto più forte dell'altra, scambia i due buzzer passivi. Se il rinforzo all'ottava sotto non ti piace, metti `RINFORZO_OTTAVA = false`.

---

## Librerie

Bastano quelle già incluse nell'IDE Arduino: `LiquidCrystal` ed `EEPROM`.
