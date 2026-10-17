/*
  CONSOLE MUSICALE CON 2 JOYSTICK E 3 BUZZER - da 1 a 4 giocatori - Arduino UNO
  ------------------------------------------------------------
  Le note si suonano spingendo i due joystick: ogni direzione e' una nota.

      JOYSTICK 1             JOYSTICK 2
          Do                     Sol
      Mi  +  Fa              Si   +  Do'
          Re                     La

  Le note vicine (Do-Re, Mi-Fa, Sol-La, Si-Do') sono in direzioni opposte:
  cosi' ogni accordo si suona con una diagonale su uno stick e una
  direzione sull'altro. Esempio: Do maggiore = joystick 1 su-sinistra
  (Do + Mi) e joystick 2 su (Sol). Puoi muovere i due stick insieme.

  TRE BUZZER
    - passivo 1 (D9): voce principale, generata dal Timer1 del chip
    - passivo 2 (D13): seconda voce, generata dal Timer2 con tone()
      Due note insieme suonano davvero insieme, una per buzzer.
      Con una nota sola la seconda voce la rinforza un'ottava sotto.
      Con 3-4 note la voce 1 tiene la nota piu' bassa e la voce 2
      alterna velocissima le altre (arpeggio).
    - attivo (A4): percussione. Batte il tempo nel juke-box, fa il
      ticchettio degli ultimi 3 secondi e il "bzzz" degli errori.
  L'LCD scrive il nome dell'accordo che stai suonando.
  Gli 8 LED (uno per nota) sono comandati dal chip 74HC595.

  MENU: l'LCD scorre i giochi, spingi lo stick nella direzione indicata.
    J1 su   -> SIMON     1-4 giocatori a turno ripetono la sequenza che si allunga
    J1 giu  -> CATENA    2-4 giocatori: ripeti a memoria e aggiungi una nota
    J1 sx   -> ORECCHIO  1-4 giocatori: Do di riferimento, poi indovina la nota
    J1 dx   -> ACCORDI   1-4 giocatori: l'LCD scrive un accordo, suonalo
    J2 su   -> PIANO     suoni liberamente, anche piu' note insieme
    J2 giu  -> JUKE-BOX  10 melodie di pubblico dominio: melodia + basso + batteria
                          (J1 su = brano dopo, J1 giu = piu' lento,
                           J1 sx = piu' veloce, J1 dx = batteria si/no)
    J2 sx   -> TEST      mostra i valori dei joystick e le note lette;
                          Do + Si per 3 s azzera il record
  Per tornare al MENU: tieni premuto lo stick del joystick 1 per 1 secondo.
  Numero di giocatori: J1 su = 1, giu = 2, sinistra = 3, destra = 4.

  COLLEGAMENTI (vedi la pagina con il disegno della breadboard)
    Joystick 1: GND -> GND, +5V -> 5V, VRx -> A0, VRy -> A1, SW -> D2
    Joystick 2: GND -> GND, +5V -> 5V, VRx -> A2, VRy -> A3 (SW non serve)
    Buzzer passivo 1: + -> D9, - -> GND   (deve essere D9: lo usa il Timer1)
    Buzzer passivo 2: + -> D13, - -> GND
    Buzzer attivo:    + -> A4, - -> GND   (quello con l'adesivo e il fondo chiuso)
    74HC595: pin 14 DS -> D10, pin 12 ST_CP (latch) -> D11, pin 11 SH_CP (clock) -> D12
             pin 16 VCC e pin 10 MR -> 5V | pin 8 GND e pin 13 OE -> GND
             uscite Q0..Q7 (pin 15, 1, 2, 3, 4, 5, 6, 7) -> LED Do..Do' -> 330 ohm -> GND
    LCD1602: RS -> D3, E -> D4, D4 -> D5, D5 -> D6, D6 -> D7, D7 -> D8
             VSS, RW, K -> GND | VDD -> 5V | A -> 220 ohm -> 5V | V0 -> potenziometro
    Pin libero: A5

  Se una direzione suona la nota sbagliata (es. su e giu' scambiati),
  usa il TEST e cambia INVERTI_X / INVERTI_Y qui sotto.

  Librerie: LiquidCrystal ed EEPROM (gia' incluse nell'IDE Arduino).
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// ===================== PIN =====================
const byte NT = 8;                     // note: Do Re Mi Fa Sol La Si Do'
const byte J1X = A0, J1Y = A1;         // joystick 1
const byte J2X = A2, J2Y = A3;         // joystick 2
const byte BTN_MENU = 2;               // pulsante (SW) del joystick 1
const byte BUZ1     = 9;               // buzzer passivo 1 (Timer1, uscita OC1A: solo D9)
const byte BUZ2     = 13;              // buzzer passivo 2 (tone, Timer2)
const byte BUZ_ATT  = A4;              // buzzer attivo (percussione)
const byte SR_DATA  = 10;              // 74HC595 pin 14
const byte SR_LATCH = 11;              // 74HC595 pin 12
const byte SR_CLOCK = 12;              // 74HC595 pin 11
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);   // RS, E, D4, D5, D6, D7

// ===================== JOYSTICK =====================
// Nota per ogni direzione: [joystick][su, giu, sinistra, destra]
const byte DIREZIONE[2][4] = { {0, 1, 2, 3},     // J1: Do, Re, Mi, Fa
                               {4, 5, 6, 7} };   // J2: Sol, La, Si, Do'
// Se su/giu' o sinistra/destra risultano scambiati, metti true
const bool INVERTI_X[2] = {false, false};
const bool INVERTI_Y[2] = {false, false};
// Il centro vale circa 512: oltre queste soglie la direzione conta come spinta.
// Sono abbastanza larghe da riconoscere anche le diagonali.
const int SOGLIA_BASSA = 260;
const int SOGLIA_ALTA  = 764;
const unsigned long TEMPO_MENU = 1000;   // ms da tenere premuto lo stick per il menu

// ===================== SUONO =====================
const bool RINFORZO_OTTAVA = true;   // nota singola: la voce 2 la raddoppia un'ottava sotto
bool batteria = true;                // il buzzer attivo batte il tempo nel juke-box

// ===================== NOTE =====================
#define G3 196
#define C4 262
#define D4 294
#define E4 330
#define F4 349
#define G4 392
#define A4 440
#define B4 494
#define C5 523

const int NOTE[NT] = {C4, D4, E4, F4, G4, A4, B4, C5};
const char* const NOME[NT] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si", "Do'"};

// Accordi a tre note sulla scala di Do (bit 0 = Do ... bit 6 = Si, bit 7 = Do')
struct Accordo { byte mask; const char* nome; };
const Accordo ACCORDI[] = {
  {0x15, "Do maggiore"},    // Do Mi Sol
  {0x2A, "Re minore"},      // Re Fa La
  {0x54, "Mi minore"},      // Mi Sol Si
  {0x29, "Fa maggiore"},    // Fa La Do
  {0x52, "Sol maggiore"},   // Sol Si Re
  {0x25, "La minore"},      // La Do Mi
  {0x4A, "Si diminuito"},   // Si Re Fa
};
const byte N_ACCORDI = sizeof(ACCORDI) / sizeof(ACCORDI[0]);

// ===================== MELODIE (tutte di pubblico dominio) =====================
// Coppie {frequenza in Hz, durata in sedicesimi}: 1 = sedicesimo, 2 = ottavo,
// 4 = quarto, 6 = quarto puntato, 8 = minima, 16 = semibreve. Frequenza 0 = pausa.
// Le melodie da Jingle Bells in poi sono prese dalla raccolta
// github.com/hibit-dev/buzzer (codice con licenza MIT, (c) 2022 HiBit).

// Fra Martino - canto tradizionale francese
const int FRA_MARTINO[] PROGMEM = {
  C4,4, D4,4, E4,4, C4,4, C4,4, D4,4, E4,4, C4,4,
  E4,4, F4,4, G4,8, E4,4, F4,4, G4,8,
  G4,2, A4,2, G4,2, F4,2, E4,4, C4,4, G4,2, A4,2, G4,2, F4,2, E4,4, C4,4,
  C4,4, G3,4, C4,8, C4,4, G3,4, C4,8 };

// Inno alla gioia - Ludwig van Beethoven, Nona sinfonia, 1824
const int INNO_GIOIA[] PROGMEM = {
  E4,4, E4,4, F4,4, G4,4, G4,4, F4,4, E4,4, D4,4,
  C4,4, C4,4, D4,4, E4,4, E4,6, D4,2, D4,8,
  E4,4, E4,4, F4,4, G4,4, G4,4, F4,4, E4,4, D4,4,
  C4,4, C4,4, D4,4, E4,4, D4,6, C4,2, C4,8 };

// Brilla stellina - melodia francese del Settecento
const int STELLINA[] PROGMEM = {
  C4,4, C4,4, G4,4, G4,4, A4,4, A4,4, G4,8,
  F4,4, F4,4, E4,4, E4,4, D4,4, D4,4, C4,8,
  G4,4, G4,4, F4,4, F4,4, E4,4, E4,4, D4,8,
  G4,4, G4,4, F4,4, F4,4, E4,4, E4,4, D4,8,
  C4,4, C4,4, G4,4, G4,4, A4,4, A4,4, G4,8,
  F4,4, F4,4, E4,4, E4,4, D4,4, D4,4, C4,8 };

// Jingle Bells - James Lord Pierpont, 1857
const int JINGLE_BELLS[] PROGMEM = {
  659,2, 659,2, 659,4, 659,2, 659,2, 659,4, 659,2, 784,2,
  523,2, 587,2, 659,8, 698,2, 698,2, 698,2, 698,2, 698,2,
  659,2, 659,2, 659,1, 659,1, 659,2, 587,2, 587,2, 659,2,
  587,4, 784,4 };

// Tanti auguri - Mildred e Patty Hill, 1893
const int TANTI_AUGURI[] PROGMEM = {
  262,4, 262,2, 294,4, 262,4, 349,4, 330,8, 262,4, 262,2,
  294,4, 262,4, 392,4, 349,8, 262,4, 262,2, 523,4, 440,4,
  349,4, 330,4, 294,4, 466,4, 466,2, 440,4, 349,4, 392,4,
  349,8 };

// Toreador (Bizet) - Georges Bizet, Carmen, 1875
const int TOREADOR[] PROGMEM = {
  523,8, 587,6, 523,2, 440,8, 440,8, 440,6, 392,2, 440,6,
  466,2, 440,16, 466,8, 392,6, 523,2, 440,16, 349,8, 294,6,
  392,2, 262,16, 392,16, 392,4, 587,4, 523,4, 466,4, 440,4,
  392,4, 440,4, 466,4, 440,16, 330,8, 440,8, 440,8, 415,6,
  494,2, 659,16, 0,16 };

// Gran Vals - Francisco Tarrega, 1902 (la suoneria Nokia)
const int GRAN_VALS[] PROGMEM = {
  659,2, 587,2, 370,4, 415,4, 554,2, 494,2, 294,4, 330,4,
  494,2, 440,2, 277,4, 330,4, 440,8 };

// Korobeiniki - canto popolare russo, 1861 (il tema di Tetris)
const int KOROBEINIKI[] PROGMEM = {
  659,4, 494,2, 523,2, 587,4, 523,2, 494,2, 440,4, 440,2,
  523,2, 659,4, 587,2, 523,2, 494,4, 523,2, 587,4, 659,4,
  523,4, 440,4, 440,2, 440,4, 494,2, 523,2, 587,4, 698,2,
  880,4, 784,2, 698,2, 659,4, 523,2, 659,4, 587,2, 523,2,
  494,4, 494,2, 523,2, 587,4, 659,4, 523,4, 440,4, 440,4,
  0,4, 659,4, 494,2, 523,2, 587,4, 523,2, 494,2, 440,4,
  440,2, 523,2, 659,4, 587,2, 523,2, 494,4, 523,2, 587,4,
  659,4, 523,4, 440,4, 440,2, 440,4, 494,2, 523,2, 587,4,
  698,2, 880,4, 784,2, 698,2, 659,4, 523,2, 659,4, 587,2,
  523,2, 494,4, 494,2, 523,2, 587,4, 659,4, 523,4, 440,4,
  440,4, 0,4, 659,8, 523,8, 587,8, 494,8, 523,8, 440,8,
  415,8, 494,4, 0,2, 659,8, 523,8, 587,8, 494,8, 523,4,
  659,4, 880,8, 831,8 };

// Passacaglia - Georg Friedrich Haendel
const int PASSACAGLIA[] PROGMEM = {
  110,2, 220,2, 330,2, 262,2, 440,2, 262,2, 330,2, 262,2,
  110,2, 220,2, 330,2, 262,2, 440,2, 262,2, 330,2, 262,2,
  523,2, 1047,2, 988,2, 1047,2, 880,2, 1047,2, 784,2, 1047,2,
  698,2, 1047,2, 659,2, 1047,2, 587,2, 1047,2, 523,2, 1047,2,
  494,2, 988,2, 880,2, 988,2, 784,2, 988,2, 698,2, 988,2,
  659,2, 988,2, 587,2, 988,2, 523,2, 988,2, 494,2, 988,2,
  440,2, 880,2, 784,2, 880,2, 698,2, 880,2, 659,2, 880,2,
  587,2, 880,2, 523,2, 880,2, 494,2, 880,2, 440,2, 880,2,
  880,4, 831,2, 740,2, 831,6, 880,2, 880,8 };

// Tri martolod - melodia tradizionale bretone
const int TRI_MARTOLOD[] PROGMEM = {
  440,4, 523,4, 587,4, 659,6, 659,6, 698,4, 587,4, 659,8,
  0,4, 587,4, 587,3, 523,3, 494,3, 523,8, 440,6, 0,2,
  440,4, 587,4, 587,4, 523,4, 494,4, 587,4, 587,3, 523,3,
  494,3, 523,8, 440,6, 0,2, 392,8, 440,16 };

// Accompagnamento sul secondo buzzer passivo
const byte BASSO_DO_FA_SOL = 0;   // basso automatico Do / Fa / Sol (brani in Do maggiore)
const byte BASSO_OTTAVA    = 1;   // la melodia raddoppiata un'ottava sotto

struct Brano {
  const char* titolo;       // massimo 16 caratteri (riga dell'LCD)
  const int*  note;         // coppie {frequenza, durata} in PROGMEM
  int         coppie;       // numero di note
  byte        msSedicesimo; // durata di un sedicesimo a velocita' normale
  byte        basso;        // tipo di accompagnamento
};
#define BRANO(t, a, ms, bs) { t, a, (int)(sizeof(a) / 4), ms, bs }
const Brano BRANI[] = {
  BRANO("Fra Martino",      FRA_MARTINO,  110, BASSO_DO_FA_SOL),
  BRANO("Inno alla gioia",  INNO_GIOIA,   110, BASSO_DO_FA_SOL),
  BRANO("Brilla stellina",  STELLINA,     110, BASSO_DO_FA_SOL),
  BRANO("Jingle Bells",     JINGLE_BELLS,  80, BASSO_DO_FA_SOL),
  BRANO("Tanti auguri",     TANTI_AUGURI,  80, BASSO_OTTAVA),
  BRANO("Toreador (Bizet)", TOREADOR,      80, BASSO_OTTAVA),
  BRANO("Gran Vals",        GRAN_VALS,     80, BASSO_OTTAVA),
  BRANO("Korobeiniki",      KOROBEINIKI,   80, BASSO_OTTAVA),
  BRANO("Passacaglia",      PASSACAGLIA,   80, BASSO_OTTAVA),
  BRANO("Tri martolod",     TRI_MARTOLOD,  80, BASSO_OTTAVA),
};
const byte N_BRANI = sizeof(BRANI) / sizeof(BRANI[0]);

// ===================== STATO =====================
const int MENU = -2;          // e' stato premuto il tasto MENU
const int NESSUNO = -1;       // tempo scaduto
const byte MAX_LEN = 100;
const unsigned long TEMPO_SIMON   = 3000;
const unsigned long TEMPO_CATENA  = 5000;
const unsigned long TEMPO_ACCORDO = 10000;
const byte GIRI_MULTI = 5;    // domande per giocatore in Orecchio e Accordi

byte sequenza[MAX_LEN];
byte record = 0;
int  velocita = 100;          // velocita' del juke-box in percentuale
byte ledAttuali = 0;
int  suonando = -1;           // nota che il buzzer sta suonando ora
byte arpIdx = 0;
unsigned long arpT = 0;

// =========================================================
//                     LED (74HC595)
// =========================================================
void mostraLed(byte m) {
  ledAttuali = m;
  digitalWrite(SR_LATCH, LOW);
  shiftOut(SR_DATA, SR_CLOCK, MSBFIRST, m);   // bit 0 -> Q0 (Do) ... bit 7 -> Q7 (Do')
  digitalWrite(SR_LATCH, HIGH);
}
void accendi(byte i, bool on) { mostraLed(on ? (ledAttuali | (1 << i)) : (ledAttuali & ~(1 << i))); }
void tutti(bool on) { mostraLed(on ? 0xFF : 0); }

// =========================================================
//                 JOYSTICK (anche piu' note insieme)
// =========================================================
// Note spinte su un joystick: una direzione = 1 nota, una diagonale = 2 note
byte leggiJoy(byte j, byte pinX, byte pinY) {
  int x = analogRead(pinX);
  int y = analogRead(pinY);
  if (INVERTI_X[j]) x = 1023 - x;
  if (INVERTI_Y[j]) y = 1023 - y;
  byte m = 0;
  if (y < SOGLIA_BASSA) m |= (1 << DIREZIONE[j][0]);   // su
  if (y > SOGLIA_ALTA)  m |= (1 << DIREZIONE[j][1]);   // giu'
  if (x < SOGLIA_BASSA) m |= (1 << DIREZIONE[j][2]);   // sinistra
  if (x > SOGLIA_ALTA)  m |= (1 << DIREZIONE[j][3]);   // destra
  return m;
}

// Maschera di tutte le note spinte in questo istante (bit 0 = Do ... bit 7 = Do')
byte leggiMaschera() {
  return leggiJoy(0, J1X, J1Y) | leggiJoy(1, J2X, J2Y);
}

// Maschera "stabile": tiene solo le note spinte anche 25 ms dopo.
// Cosi' le diagonali attraversate per sbaglio mentre muovi lo stick non contano.
byte maschera() {
  byte a = leggiMaschera();
  if (!a) return 0;
  delay(25);
  return a & leggiMaschera();
}

bool menuPremuto() { return digitalRead(BTN_MENU) == LOW; }

// true se lo stick del joystick 1 e' tenuto premuto per TEMPO_MENU.
// Un clic breve (capita spingendo forte lo stick) viene ignorato.
bool menuTenuto() {
  if (!menuPremuto()) return false;
  unsigned long t0 = millis();
  while (menuPremuto()) {
    if (millis() - t0 > TEMPO_MENU) return true;
  }
  return false;
}

// Aspetta che entrambi gli stick tornino al centro e lo stick 1 sia rilasciato
void rilasciaTutto() {
  while (leggiMaschera() || menuPremuto()) {}
  delay(40);
}

byte contaBit(byte m) { byte c = 0; while (m) { c += m & 1; m >>= 1; } return c; }
int primo(byte m) { for (byte i = 0; i < NT; i++) if (m & (1 << i)) return i; return -1; }

// Aspetta una nota: 0-7, MENU oppure NESSUNO (tempo scaduto)
// Se il tempo e' di almeno 5 s, il buzzer attivo ticchetta negli ultimi 3 secondi.
int aspettaTasto(unsigned long timeout) {
  unsigned long t0 = millis();
  byte tic = 3;
  while (millis() - t0 < timeout) {
    if (menuTenuto()) { zitto(); tutti(false); rilasciaTutto(); return MENU; }
    byte m = maschera();
    if (m) return primo(m);
    if (timeout >= 5000 && tic > 0 && millis() - t0 >= timeout - tic * 1000UL) { colpo(8); tic--; }
  }
  return NESSUNO;
}

// =========================================================
//                     SUONO
// =========================================================
// ---------- Voce 1: buzzer passivo su D9, onda quadra dal Timer1 ----------
// Il Timer1 conta da 0 a OCR1A e a ogni giro inverte il pin D9 (modo CTC).
// Frequenza = 16 MHz / (2 * 8 * (OCR1A + 1))
unsigned int f1 = 0, f2 = 0;

void voce1(unsigned int f) {
  if (f == f1) return;
  f1 = f;
  if (!f) {
    TCCR1A = 0; TCCR1B = 0;
    digitalWrite(BUZ1, LOW);
    return;
  }
  TCCR1A = _BV(COM1A0);                  // inverte OC1A (D9) a ogni confronto
  TCCR1B = _BV(WGM12) | _BV(CS11);       // modo CTC, prescaler 8
  OCR1A  = (F_CPU / 16UL) / f - 1;
  TCNT1  = 0;
}

// ---------- Voce 2: buzzer passivo su D13 con tone() (Timer2) ----------
void voce2(unsigned int f) {
  if (f == f2) return;
  f2 = f;
  if (f) tone(BUZ2, f); else noTone(BUZ2);
}

void voci(unsigned int a, unsigned int b) { voce1(a); voce2(b); }

// Una nota sola, rinforzata un'ottava sotto dalla seconda voce
void nota(unsigned int f) { voci(f, RINFORZO_OTTAVA ? f / 2 : 0); }

// ---------- Buzzer attivo: percussione ----------
unsigned long clickFine = 0;
void colpo(int ms) { digitalWrite(BUZ_ATT, HIGH); delay(ms); digitalWrite(BUZ_ATT, LOW); }
void click(byte ms) { digitalWrite(BUZ_ATT, HIGH); clickFine = millis() + ms; }   // non blocca
void aggiornaClick() {
  if (clickFine && (long)(millis() - clickFine) >= 0) { digitalWrite(BUZ_ATT, LOW); clickFine = 0; }
}

void zitto() {
  voci(0, 0);
  digitalWrite(BUZ_ATT, LOW);
  clickFine = 0;
  suonando = -1;
}

void suona(byte i, int ms) {
  accendi(i, true);
  nota(NOTE[i]);
  delay(ms);
  zitto();
  accendi(i, false);
}

// Suona le note della maschera sui due buzzer. Va chiamata continuamente.
//   1 nota   -> voce 1 la nota, voce 2 la stessa un'ottava sotto
//   2 note   -> una per buzzer, davvero insieme
//   3-4 note -> voce 1 tiene la piu' bassa, voce 2 alterna le altre ogni 35 ms
void suonaMaschera(byte m) {
  byte n = contaBit(m);
  if (n == 0) { if (suonando != -1) zitto(); return; }
  int basso = primo(m);
  suonando = basso;
  if (n == 1) { nota(NOTE[basso]); return; }

  byte resto = m & ~(1 << basso);
  voce1(NOTE[basso]);
  if (contaBit(resto) == 1) { voce2(NOTE[primo(resto)]); return; }

  if (!(resto & (1 << arpIdx)) || millis() - arpT >= 35) {
    for (byte k = 1; k <= NT; k++) {
      byte j = (arpIdx + k) % NT;
      if (resto & (1 << j)) { arpIdx = j; break; }
    }
    arpT = millis();
  }
  voce2(NOTE[arpIdx]);
}

// Suona un accordo per "ms" millisecondi con le sue luci
void suonaAccordo(byte m, int ms) {
  mostraLed(m);
  unsigned long t0 = millis();
  while (millis() - t0 < (unsigned long)ms) suonaMaschera(m);
  zitto();
  mostraLed(0);
}

void feedback(byte i) {
  accendi(i, true);
  nota(NOTE[i]);
  unsigned long t0 = millis();
  while ((leggiMaschera() & (1 << i)) && millis() - t0 < 1500) {}
  zitto();
  accendi(i, false);
  rilasciaTutto();
}

// Nota della scala piu' vicina a una frequenza (in qualsiasi ottava)
byte notaPerFreq(int f) {
  long g = f;
  while (g < 255) g *= 2;
  while (g > 505) g /= 2;
  byte best = 0;
  for (byte i = 1; i < NT; i++)
    if (abs(g - NOTE[i]) < abs(g - NOTE[best])) best = i;
  return best;
}

// Nome dell'accordo per una maschera, oppure NULL
const char* nomeAccordo(byte m) {
  for (byte k = 0; k < N_ACCORDI; k++) if (ACCORDI[k].mask == m) return ACCORDI[k].nome;
  return NULL;
}

// ---------------- Effetti sonori ----------------
void sigla() {
  for (byte i = 0; i < NT; i++) suona(i, 80);
  suonaAccordo(0x15, 500);   // Do maggiore
}

// Errore: il buzzer attivo ronza mentre i passivi scendono a mezzo tono di distanza
void suonoErrore() {
  tutti(true);
  digitalWrite(BUZ_ATT, HIGH);
  voci(196, 208); delay(250);
  voci(147, 156); delay(250);
  digitalWrite(BUZ_ATT, LOW);
  voci(98, 104);  delay(600);
  zitto();
  tutti(false);
}

// Giusto: due note in terza, una per buzzer
void suonoGiusto() {
  voci(784, 988);  delay(90);
  voci(1047, 1319); delay(140);
  zitto();
}

// Fanfara a due voci con il colpo di percussione su ogni nota
void fanfara() {
  const int a[] = {523, 659, 784, 1047, 784, 1047};
  const int b[] = {659, 784, 988, 1319, 988, 1319};
  const int d[] = {120, 120, 120, 250, 120, 400};
  for (byte k = 0; k < 6; k++) {
    mostraLed(k % 2 ? 0xAA : 0x55);
    voci(a[k], b[k]);
    colpo(12);
    delay(d[k] - 12);
  }
  zitto();
  tutti(false);
}

// =========================================================
//                     DISPLAY
// =========================================================
void pulisci(byte riga) {
  lcd.setCursor(0, riga);
  lcd.print(F("                "));
  lcd.setCursor(0, riga);
}

void schermo(const __FlashStringHelper* r1, const __FlashStringHelper* r2) {
  lcd.clear();
  lcd.print(r1);
  lcd.setCursor(0, 1);
  lcd.print(r2);
}

void scriviGiocatore(byte g) { lcd.print('G'); lcd.print(g + 1); }

// Scrive sulla riga 2 il nome dell'accordo o l'elenco delle note premute
void scriviNote(byte m) {
  pulisci(1);
  const char* acc = nomeAccordo(m);
  if (acc) { lcd.print(acc); return; }
  byte col = 0;
  for (byte i = 0; i < NT; i++) {
    if (m & (1 << i)) {
      byte l = strlen(NOME[i]);
      if (col + l > 16) break;
      lcd.print(NOME[i]);
      col += l;
      if (col < 16) { lcd.print(' '); col++; }
    }
  }
}

// =========================================================
//                  MENU E SCELTA GIOCATORI
// =========================================================
const byte N_VOCI = 7;

byte menu() {
  const __FlashStringHelper* voci[N_VOCI] = {
    F("J1 su  Simon"), F("J1 giu Catena"), F("J1 sx  Orecchio"), F("J1 dx  Accordi"),
    F("J2 su  Piano"), F("J2 giu Juke-box"), F("J2 sx  Test") };
  lcd.clear();
  lcd.print(F("Scegli il gioco"));
  byte i = 0;
  while (true) {
    pulisci(1);
    lcd.print(voci[i]);
    mostraLed(1 << i);
    unsigned long t0 = millis();
    while (millis() - t0 < 1500) {
      byte m = maschera();
      if (m) {
        int t = primo(m);
        if (t < N_VOCI) {
          tutti(false);
          randomSeed(micros());     // il momento della scelta rende ogni partita diversa
          suona(t, 150);
          rilasciaTutto();
          return t;
        }
      }
    }
    i = (i + 1) % N_VOCI;
  }
}

// Restituisce il numero di giocatori (minimo-4), oppure 0 se si preme MENU
byte scegliGiocatori(byte minimo) {
  lcd.clear();
  lcd.print(F("Quanti giocatori"));
  lcd.setCursor(0, 1);
  if (minimo == 1) lcd.print(F("su1 giu2 sx3 dx4"));
  else             lcd.print(F("giu2 sx3 dx4"));
  byte m = 0;
  for (byte i = minimo - 1; i < 4; i++) m |= (1 << i);
  mostraLed(m);
  while (true) {
    int t = aspettaTasto(600000UL);
    if (t == MENU || t == NESSUNO) { tutti(false); return 0; }
    if (t >= minimo - 1 && t < 4) {
      tutti(false);
      suona(t, 150);
      rilasciaTutto();
      return t + 1;
    }
  }
}

// Dopo una partita: true = rigioca, false = torna al menu
bool dopoPartita() {
  schermo(F("Spingi: rigioca"), F("Tieni J1: menu"));
  int t = aspettaTasto(60000UL);
  if (t >= 0) { rilasciaTutto(); return true; }
  return false;
}

void vincitore(bool vivo[], byte n) {
  for (byte g = 0; g < n; g++) {
    if (vivo[g]) {
      lcd.clear();
      lcd.print(F("Vince ")); scriviGiocatore(g); lcd.print('!');
      fanfara();
      delay(2000);
      return;
    }
  }
}

// Elimina il giocatore g; restituisce true se la partita e' finita
bool elimina(byte g, int premuto, int atteso, bool vivo[], byte &vivi, byte n) {
  suonoErrore();
  lcd.clear();
  scriviGiocatore(g); lcd.print(F(" eliminato"));
  lcd.setCursor(0, 1);
  if (premuto == NESSUNO) lcd.print(F("tempo scaduto"));
  else if (atteso >= 0) { lcd.print(F("era il ")); lcd.print(NOME[atteso]); }
  if (atteso >= 0) { accendi(atteso, true); delay(1500); accendi(atteso, false); }
  else delay(1500);
  vivo[g] = false;
  vivi--;
  if (vivi <= 1) { vincitore(vivo, n); return true; }
  return false;
}

// Classifica finale per i giochi a punti
void classifica(byte punti[], byte n, byte massimo) {
  lcd.clear();
  if (n == 1) {
    lcd.print(F("Punti: ")); lcd.print(punti[0]); lcd.print('/'); lcd.print(massimo);
    if (punti[0] >= massimo - 1) fanfara(); else delay(1500);
  } else {
    byte best = 0, pari = 0;
    for (byte g = 1; g < n; g++) if (punti[g] > punti[best]) best = g;
    for (byte g = 0; g < n; g++) if (punti[g] == punti[best]) pari++;
    if (pari > 1) lcd.print(F("Pareggio!"));
    else { lcd.print(F("Vince ")); scriviGiocatore(best); lcd.print('!'); }
    lcd.setCursor(0, 1);
    for (byte g = 0; g < n; g++) { lcd.print(g + 1); lcd.print(':'); lcd.print(punti[g]); lcd.print(' '); }
    fanfara();
  }
  delay(3000);
}

// =========================================================
//                 SIMON (1-4 giocatori)
// =========================================================
void simon() {
  byte n = scegliGiocatori(1);
  if (!n) return;

  do {
    bool vivo[4] = {true, true, true, true};
    byte vivi = n, len = 0, turno = 0;
    bool nuovaNota = true;

    lcd.clear();
    lcd.print(F("SIMON  ")); lcd.print(n); lcd.print(F(" gioc."));
    lcd.setCursor(0, 1);
    if (n == 1) { lcd.print(F("Record: ")); lcd.print(record); }
    else lcd.print(F("Inizia G1"));
    sigla();
    delay(800);

    bool fine = false;
    while (!fine) {
      while (!vivo[turno]) turno = (turno + 1) % n;
      if (nuovaNota && len < MAX_LEN) sequenza[len++] = random(NT);
      nuovaNota = true;

      lcd.clear();
      if (n == 1) {
        lcd.print(F("Livello ")); lcd.print(len);
        lcd.setCursor(10, 0); lcd.print(F("Rec ")); lcd.print(record);
      } else {
        scriviGiocatore(turno); lcd.print(F(" - livello ")); lcd.print(len);
      }
      lcd.setCursor(0, 1); lcd.print(F("Ascolta..."));

      int durata = max(140, 480 - len * 22);
      for (byte k = 0; k < len; k++) {
        suona(sequenza[k], durata);
        delay(durata / 3);
      }

      pulisci(1); lcd.print(F("Tocca a te!"));
      for (byte k = 0; k < len; k++) {
        int t = aspettaTasto(TEMPO_SIMON);
        if (t == MENU) return;
        if (t != sequenza[k]) {
          if (n == 1) {
            suonoErrore();
            lcd.clear();
            if (t == NESSUNO) lcd.print(F("Tempo scaduto!"));
            else { lcd.print(F("Era il ")); lcd.print(NOME[sequenza[k]]); }
            lcd.setCursor(0, 1); lcd.print(F("Punti: ")); lcd.print(len - 1);
            accendi(sequenza[k], true); delay(1500); accendi(sequenza[k], false);
            if (len - 1 > record) {
              record = len - 1;
              EEPROM.update(0, record);
              schermo(F("NUOVO RECORD!"), F(""));
              lcd.setCursor(0, 1); lcd.print(record); lcd.print(F(" note"));
              fanfara();
              delay(1500);
            }
            fine = true;
          } else {
            fine = elimina(turno, t, sequenza[k], vivo, vivi, n);
            nuovaNota = false;          // il prossimo ripete la stessa sequenza
          }
          break;
        }
        feedback(t);
      }
      if (fine) break;
      if (nuovaNota) suonoGiusto();
      turno = (turno + 1) % n;
      delay(700);
    }
  } while (dopoPartita());
}

// =========================================================
//                 CATENA (2-4 giocatori)
// =========================================================
void catena() {
  byte n = scegliGiocatori(2);
  if (!n) return;

  do {
    bool vivo[4] = {true, true, true, true};
    byte vivi = n, len = 0, turno = 0;
    schermo(F("CATENA"), F("G1 suona 1 nota"));
    delay(1500);

    bool fine = false;
    while (!fine) {
      while (!vivo[turno]) turno = (turno + 1) % n;
      lcd.clear();
      scriviGiocatore(turno); lcd.print(F(" - catena ")); lcd.print(len);

      bool ok = true;
      if (len > 0) {
        lcd.setCursor(0, 1); lcd.print(F("Ripeti a memoria"));
        for (byte k = 0; k < len; k++) {
          int t = aspettaTasto(TEMPO_CATENA);
          if (t == MENU) return;
          if (t != sequenza[k]) {
            fine = elimina(turno, t, sequenza[k], vivo, vivi, n);
            ok = false;
            break;
          }
          feedback(t);
        }
      }
      if (!ok) { turno = (turno + 1) % n; continue; }

      pulisci(1); lcd.print(F("Aggiungi 1 nota"));
      int t = aspettaTasto(TEMPO_CATENA);
      if (t == MENU) return;
      if (t == NESSUNO) {
        fine = elimina(turno, NESSUNO, -1, vivo, vivi, n);
      } else {
        feedback(t);
        if (len < MAX_LEN) sequenza[len++] = t;
        suonoGiusto();
      }
      turno = (turno + 1) % n;
      delay(500);
    }
  } while (dopoPartita());
}

// =========================================================
//               ORECCHIO MUSICALE (1-4 giocatori)
// =========================================================
void orecchio() {
  byte n = scegliGiocatori(1);
  if (!n) return;

  do {
    byte punti[4] = {0, 0, 0, 0};
    byte giri = (n == 1) ? 10 : GIRI_MULTI;

    schermo(F("ORECCHIO"), F("Ecco le 8 note"));
    for (byte i = 0; i < NT; i++) {
      pulisci(1); lcd.print(NOME[i]);
      suona(i, 400); delay(120);
    }
    delay(600);

    for (byte giro = 0; giro < giri; giro++) {
      for (byte g = 0; g < n; g++) {
        byte mistero = random(NT);
        lcd.clear();
        scriviGiocatore(g); lcd.print(F("  nota ")); lcd.print(giro + 1);
        lcd.print('/'); lcd.print(giri);
        lcd.setCursor(0, 1); lcd.print(F("Do... e poi?"));
        delay(700);

        suona(0, 500);                                   // Do di riferimento con la luce
        delay(350);
        voci(NOTE[mistero], 0); delay(700); zitto();     // nota misteriosa al buio, voce sola

        pulisci(1); lcd.print(F("Quale nota e'?"));
        int t = aspettaTasto(10000UL);
        if (t == MENU) return;
        if (t >= 0) rilasciaTutto();

        pulisci(1);
        if (t == mistero) {
          punti[g]++;
          lcd.print(F("Giusto! ")); lcd.print(NOME[mistero]);
          accendi(mistero, true); suonoGiusto(); delay(700); accendi(mistero, false);
        } else {
          lcd.print(F("Era il ")); lcd.print(NOME[mistero]);
          suona(mistero, 700);
          delay(500);
        }
      }
    }
    classifica(punti, n, giri);
  } while (dopoPartita());
}

// =========================================================
//               ACCORDI (1-4 giocatori) - tasti insieme!
// =========================================================
void accordi() {
  byte n = scegliGiocatori(1);
  if (!n) return;

  do {
    byte punti[4] = {0, 0, 0, 0};
    byte giri = (n == 1) ? 10 : GIRI_MULTI;

    // Lezione: Do maggiore = J1 su-sinistra (Do + Mi) e J2 su (Sol)
    schermo(F("Do maggiore"), F("Do Mi Sol"));
    suonaAccordo(0x15, 1200);
    pulisci(1); lcd.print(F("J1 su-sx + J2 su"));
    suonaAccordo(0x15, 1500);
    delay(500);

    for (byte giro = 0; giro < giri; giro++) {
      for (byte g = 0; g < n; g++) {
        const Accordo& a = ACCORDI[random(N_ACCORDI - 1)];   // senza il diminuito
        lcd.clear();
        scriviGiocatore(g); lcd.print(F("  accordo ")); lcd.print(giro + 1);
        lcd.setCursor(0, 1); lcd.print(a.nome);

        // Aspetta che il giocatore tenga premuti 3 tasti per mezzo secondo
        byte risposta = 0, ultimo = 0, tic = 3;
        unsigned long stabileDa = 0, t0 = millis();
        while (millis() - t0 < TEMPO_ACCORDO) {
          if (menuTenuto()) { zitto(); tutti(false); rilasciaTutto(); return; }
          if (tic > 0 && millis() - t0 >= TEMPO_ACCORDO - tic * 1000UL) { colpo(8); tic--; }
          byte m = maschera();
          mostraLed(m);
          suonaMaschera(m);
          if (contaBit(m) >= 3) {
            if (m != ultimo) { ultimo = m; stabileDa = millis(); }
            else if (millis() - stabileDa > 500) { risposta = m; break; }
          } else ultimo = 0;
        }
        zitto();
        mostraLed(0);
        rilasciaTutto();

        lcd.clear();
        if (risposta == a.mask) {
          punti[g]++;
          lcd.print(F("Giusto!"));
          lcd.setCursor(0, 1); lcd.print(a.nome);
          suonoGiusto();
          suonaAccordo(a.mask, 700);
        } else {
          lcd.print(risposta ? F("No, era:") : F("Tempo! Era:"));
          lcd.setCursor(0, 1);
          for (byte i = 0; i < NT; i++)
            if (a.mask & (1 << i)) { lcd.print(NOME[i]); lcd.print(' '); }
          for (byte i = 0; i < NT; i++) if (a.mask & (1 << i)) suona(i, 300);
          suonaAccordo(a.mask, 900);
        }
        delay(600);
      }
    }
    classifica(punti, n, giri);
  } while (dopoPartita());
}

// =========================================================
//                PIANOFORTE (anche accordi)
// =========================================================
void pianoforte() {
  schermo(F("PIANO"), F("tieni J1 = menu"));
  byte prima = 0xFF;
  while (true) {
    if (menuTenuto()) { zitto(); tutti(false); rilasciaTutto(); return; }
    byte m = maschera();
    suonaMaschera(m);
    if (m != prima) {
      mostraLed(m);
      scriviNote(m);
      prima = m;
    }
  }
}

// =========================================================
//                        JUKE-BOX
// =========================================================
// Basso automatico: la fondamentale dell'accordo di Do, Fa o Sol che contiene
// la nota della melodia, un'ottava sotto (sul secondo buzzer passivo)
unsigned int bassoPer(int f) {
  switch (notaPerFreq(f)) {
    case 0: case 2: case 4: case 7: return 131;   // Do, Mi, Sol -> Do
    case 3: case 5:                 return 175;   // Fa, La      -> Fa
    default:                        return 196;   // Re, Si      -> Sol
  }
}

void intestazioneBrano(byte b) {
  lcd.clear();
  lcd.print(F("BRANO ")); lcd.print(b + 1); lcd.print('/'); lcd.print(N_BRANI);
  lcd.setCursor(12, 0); lcd.print(batteria ? F("+bat") : F("    "));
  lcd.setCursor(0, 1); lcd.print(BRANI[b].titolo);
}

// Restituisce: 0 = brano finito, 1 = salta al successivo, 2 = torna al menu
byte suonaBrano(byte b) {
  intestazioneBrano(b);
  const Brano& br = BRANI[b];
  unsigned long pos = 0;      // posizione nel brano, in sedicesimi
  long ultimoBattito = -1;

  for (int k = 0; k < br.coppie; k++) {
    int f = pgm_read_word(&br.note[2 * k]);
    int d = pgm_read_word(&br.note[2 * k + 1]);
    unsigned long passo = (unsigned long)br.msSedicesimo * 100 / velocita;   // ms per sedicesimo
    unsigned long ms = d * passo;

    if (f == 0) {                                    // pausa
      voci(0, 0);
      mostraLed(0);
    } else {
      mostraLed(1 << notaPerFreq(f));
      unsigned int acc;
      if (br.basso == BASSO_DO_FA_SOL) acc = bassoPer(f);
      else acc = (f / 2 >= 100) ? f / 2 : 0;          // sotto 100 Hz il buzzer non si sente
      voci(f, acc);                                  // melodia sul passivo 1, accompagnamento sul 2
    }

    unsigned long t0 = millis();
    while (millis() - t0 < ms) {
      // batteria: un colpo ogni quarto (4 sedicesimi), piu' lungo a inizio battuta
      long o = pos + (millis() - t0) / passo;
      if (batteria && o % 4 == 0 && o != ultimoBattito) {
        click(o % 16 == 0 ? 25 : 8);
        ultimoBattito = o;
      }
      aggiornaClick();
      if (millis() - t0 >= ms * 9 / 10) voce1(0);   // piccola pausa fra le note

      if (menuTenuto()) { zitto(); tutti(false); rilasciaTutto(); return 2; }
      byte m = leggiMaschera();
      if (m & 0x01) { zitto(); tutti(false); rilasciaTutto(); return 1; }        // J1 su: brano dopo
      if (m & 0x02) { velocita = max(50, velocita - 10);  rilasciaTutto(); }     // J1 giu: piu' lento
      if (m & 0x04) { velocita = min(200, velocita + 10); rilasciaTutto(); }     // J1 sx: piu' veloce
      if (m & 0x08) {                                                           // J1 dx: batteria
        batteria = !batteria;
        lcd.setCursor(12, 0); lcd.print(batteria ? F("+bat") : F("    "));
        rilasciaTutto();
      }
    }
    pos += d;
    mostraLed(0);
  }
  zitto();
  delay(900);
  return 0;
}

void jukebox() {
  byte b = 0;
  while (true) {
    if (suonaBrano(b) == 2) return;
    b = (b + 1) % N_BRANI;
  }
}

// =========================================================
//                       TEST TASTI
// =========================================================
void testTasti() {
  schermo(F("TEST  tieni J1"), F("per uscire"));
  delay(1200);
  byte prima = 0xFF;
  unsigned long resetDa = 0, ultimoLcd = 0;
  while (true) {
    if (menuTenuto()) { zitto(); tutti(false); rilasciaTutto(); return; }
    byte m = maschera();
    mostraLed(m);
    suonaMaschera(m);

    // riga 1: valori grezzi dei due joystick (x,y del J1 e del J2)
    if (millis() - ultimoLcd > 250 || m != prima) {
      ultimoLcd = millis();
      int v[4] = {analogRead(J1X), analogRead(J1Y), analogRead(J2X), analogRead(J2Y)};
      char riga[17];                     // valori in decine per stare nei 16 caratteri
      snprintf(riga, sizeof(riga), "%3d %3d %3d %3d", v[0] / 10, v[1] / 10, v[2] / 10, v[3] / 10);
      lcd.setCursor(0, 0);
      lcd.print(riga);
      if (m != prima) {
        if (m) scriviNote(m); else { pulisci(1); lcd.print(F("al centro")); }
        Serial.print(F("J1 x,y = ")); Serial.print(v[0]); Serial.print(','); Serial.print(v[1]);
        Serial.print(F("   J2 x,y = ")); Serial.print(v[2]); Serial.print(','); Serial.print(v[3]);
        Serial.print(F("   note: "));
        for (byte i = 0; i < NT; i++) if (m & (1 << i)) { Serial.print(NOME[i]); Serial.print(' '); }
        Serial.println();
        prima = m;
      }
    }

    // Do + Si tenuti insieme per 3 secondi: azzera il record di Simon
    if (m == 0x41) {
      if (!resetDa) resetDa = millis();
      else if (millis() - resetDa > 3000) {
        zitto();
        record = 0;
        EEPROM.update(0, 0);
        schermo(F("Record azzerato"), F(""));
        delay(1500);
        resetDa = 0;
        prima = 0xFF;
      }
    } else resetDa = 0;
  }
}

// =========================================================
void setup() {
  pinMode(BTN_MENU, INPUT_PULLUP);    // il pulsante del joystick non ha resistenza propria
  pinMode(BUZ1, OUTPUT);
  pinMode(BUZ2, OUTPUT);
  pinMode(BUZ_ATT, OUTPUT);
  digitalWrite(BUZ_ATT, LOW);
  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  mostraLed(0);
  Serial.begin(9600);
  lcd.begin(16, 2);

  record = EEPROM.read(0);
  if (record == 255) record = 0;

  schermo(F("CONSOLE JOYSTICK"), F("3 buzzer, 1-4 g."));
  sigla();
  delay(1000);
}

void loop() {
  tutti(false);
  switch (menu()) {
    case 0: simon();      break;
    case 1: catena();     break;
    case 2: orecchio();   break;
    case 3: accordi();    break;
    case 4: pianoforte(); break;
    case 5: jukebox();    break;
    case 6: testTasti();  break;
  }
}
