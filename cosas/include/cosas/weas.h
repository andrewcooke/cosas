
#ifndef COSAS_WEAS_H
#define COSAS_WEAS_H


// common constants shared by cosas and weas
// defined in cosas since weas depends on cosas and not vice-versa


enum Channel {Left, Right};
static constexpr uint N_CHANNELS = Right + 1;
enum Knob { Main, X, Y, Switch };
enum SwitchPosition { Down, Middle, Up };
static constexpr uint N_KNOBS = Switch + 1;


#endif
