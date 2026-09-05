// c_ext demo: typedef, enum, struct, union, sizeof, erweiterte Typen
typedef unsigned char u8;
typedef int u4;

u8 wert;                // = char wert;
char name1[3];                 // normales 3-Nibble-Array
u4 zaehler;             // = char zaehler;

enum { AUS, AN = 5, FERTIG } zustand;

struct Punkt { char x; char y; };
struct Punkt p;

union Zahl { char lo; char hi; };
union Zahl u;

void main(void) {
    wert = 3;
    zaehler = 12;
    zustand = FERTIG;
    p.x = 4;
    p.y = 9;
    u.lo = 7;

    print_dec(wert);
    print_dec(zaehler);
    print_dec(zustand);
    print_dec(p.x);
    print_dec(p.y);
    print_dec(u.hi);            // Union: gleicher Nibble wie lo -> 7
    print_dec(sizeof(struct Punkt));
    print_dec(sizeof(union Zahl));
    print_dec(sizeof(char));
    print_dec(sizeof(name1));   // 3 Nibbles

    wert = 0;
    while (wert < 15) {
        wert = wert + 1;
    }
    while (1) { }       // Ende: Gast haelt an
}
