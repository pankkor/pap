#pragma once

#include "sim86_types.h"

// Data stream
struct stream {
  u8 * restrict data;
  u8 * restrict end;
};

// Read byte if not end of stream and adjust data pointer of a stream
bool stream_read_b(struct stream *s, u8 * restrict out_byte);

// Read word if not end of stream and adjust data pointer of a stream
bool stream_read_w(struct stream *s, u16 * restrict out_word);

// Read byte or word if not end of stream and adjust data pointer of a stream
bool stream_read(struct stream *s, u16 * restrict out_word, bool is_word);

// Read byte or word if not end of stream and adjust data pointer of a stream.
// If byte is read it's is sign extended to a word
bool stream_read_sign(struct stream *s, i16 * restrict out_word, bool is_word);
