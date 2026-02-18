#include "key.hpp"

Key keys[MAX_KEYS];

/*
 *  s d   g h j
 * z x c v b n m ,
 * | | | | | | | |
 * C D E F G A B C
 */
key_t Key::char_to_key(char c) {
  switch (c) {
  case ';':
    return E4;
  case 'p':
    return Dh4;
  case 'l':
    return D4;
  case 'o':
    return Ch4;
  case 'k':
    return C4;
  case 'j':
    return B4;
  case 'u':
    return Bb4;
  case 'h':
    return A4;
  case 'y':
    return Ab3;
  case 'g':
    return G3;
  case 't':
    return Fh3;
  case 'f':
    return F3;
  case 'd':
    return E3;
  case 'e':
    return Dh3;
  case 's':
    return D3;
  case 'w':
    return Ch3;
  case 'a':
    return C3;
  default:
    return A3;
  }
}

float Key::get_freq() {
  switch (key) {
  case A3:
    return 200.0;
  case Bb3:
    return 233.08;
  case B3:
    return 246.94;
  case C3:
    return 261.626;
  case Ch3:
    return 277.183;
  case D3:
    return 293.665;
  case Dh3:
    return 311.127;
  case E3:
    return 329.628;
  case F3:
    return 349.228;
  case Fh3:
    return 369.994;
  case G3:
    return 391.995;
  case Ab3:
    return 415.305;
  case A4:
    return 440.000;
  case Bb4:
    return 466.164;
  case B4:
    return 493.883;
  case C4:
    return 523.251;
  default:
    return 0.;
  }
}