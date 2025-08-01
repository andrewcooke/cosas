
#include <cosas/buffer.h>


AudioBuffer::AudioBuffer(std::function<int16_t(int32_t)>& cb)
  : callback(cb), read_from(0), write_to(0), buffer() {
    // start with array full of EMPTY values
    buffer.fill(EMPTY);
    write_to = BUFFER_LEN - 1;
};

int16_t AudioBuffer::next() {
    if (read_from != write_to) {
        int16_t ret = buffer[read_from];
        read_from = (read_from + 1u) % BUFFER_LEN;
        return ret;
    } else {
        return EMPTY;
    }
}

void AudioBuffer::run() {
    // TODO - edited this and am unsure if i broken anything subtle
    while (true) {
        const size_t saved_read_from = read_from;
        int space = static_cast<int>((saved_read_from - write_to - 1) % BUFFER_LEN);
        if (space == 0) {
            while (saved_read_from == read_from) sleep_us(1);  // 44hz is 20us
        } else {
            int n = std::max(1, space - 1);  // space is 1, n is 1; 2, n is 1; 3, n is 2; etc
            log_delta(n);
            int source = static_cast<int>((write_to - 1) % BUFFER_LEN);
            for (int pad = 0; pad < n-1; pad++) {
                buffer[write_to++] = buffer[source];
                write_to %= BUFFER_LEN;
            }
            buffer[write_to++] = callback(n);
            write_to %= BUFFER_LEN;
        }
    }
}
