
#include <cosas/buffer.h>


Buffer::Buffer(std::function<int16_t(int32_t)>& cb, std::function<void(uint32_t)>& slp)
  : callback(cb), sleep_us(slp), buffer(), read_from(0), write_to(0) {
    // start with array full of EMPTY values
    buffer.fill(EMPTY);
    write_to = BUFFER_LEN - 1;
};

int16_t Buffer::next() {
    if (read_from != write_to) {
        int16_t ret = buffer[read_from++];
        if (read_from == BUFFER_LEN) read_from = 0;
        return ret;
    } else {
        return EMPTY;
    }
}

void Buffer::run() {
    while (true) {
        int saved_read_from = read_from;
        int space = saved_read_from - write_to - 1;
        if (space < 0) space += BUFFER_LEN;
        if (space == 0) {
            while (saved_read_from == read_from) sleep_us(1);  // 44hz is 20us
        } else if (space < 3) {  // allow some catch-up without padding
            buffer[write_to++] = callback(1);
            write_to %= BUFFER_LEN;
        } else {
            int n = space - 1;  // space is at least 3
            int source = (write_to - 1) % static_cast<int>(BUFFER_LEN);
            for (int pad = 0; pad < n-1; pad++) {
                buffer[write_to++] = buffer[source];
                write_to %= BUFFER_LEN;
            }
            buffer[write_to++] = callback(n);
            write_to %= BUFFER_LEN;
        }
    }
}

