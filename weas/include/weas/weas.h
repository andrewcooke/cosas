
#ifndef WEAS_H
#define WEAS_H


// code common across multiple modules
// at some point i will enclose this in a namespace,
// but when i last tried gcc didn't really support them


enum Channel {Left, Right};
static constexpr uint N_CHANNELS = Channel::Right + 1;

#endif
