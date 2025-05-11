
#ifndef COSA_NODE_H
#define COSA_NODE_H

import std;
using namespace std;

#include "source.h"


// nodes are sources that are connected into an engine.

// at some point they will include support for confuration (and likely
// this will no longer be empty)

class Node : public Source {};

#endif
