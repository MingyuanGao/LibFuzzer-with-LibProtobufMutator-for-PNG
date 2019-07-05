#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h> // for crc32

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

/* Normally use <png.h> here to get the installed libpng, but this is done to
 * ensure the code picks up the local libpng implementation!
 */
#include "libpng/png.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "png.pb.h"
#include "png_proto_util.h"

static void WriteInt(std::stringstream &out, uint32_t x) {
  x = __builtin_bswap32(x);
  out.write((char *)&x, sizeof(x));
}

static void WriteByte(std::stringstream &out, uint8_t x) {
  out.write((char *)&x, sizeof(x));
}

static std::string Compress(const std::string &s) {
  std::string out(s.size() + 100, '\0');
  size_t out_len = out.size();
  compress((uint8_t *)&out[0], &out_len, (uint8_t *)s.data(), s.size());
  out.resize(out_len);
  return out;
}

// Chunk is written as:
//  * 4-byte length
//  * 4-byte type
//  * the data itself
//  * 4-byte crc (of type and data)
static void WriteChunk(std::stringstream &out, const char *type,
                       const std::string &chunk, bool compress) {
  std::string compressed;
  const std::string *s = &chunk;
  if (compress) {
    compressed = Compress(chunk);
    s = &compressed;
  }
  uint32_t len = s->size();
  uint32_t crc = crc32(crc32(0, (const unsigned char *)type, 4),
                       (const unsigned char *)s->data(), s->size());

  WriteInt(out, len);              // length
  out.write(type, 4);              // type
  out.write(s->data(), s->size()); // data
  WriteInt(out, crc);              // crc
}

/* Convert png file to protobuf file
 * @in_file: png file
 * @out_file: protobuf file
 */
#define MAX_CHUNK_LENGTH 1000000
/* Read one character (inchar), return octet (c), break if EOF */
#define GETBREAK                                                               \
  {                                                                            \
    inchar = getc(fp_in);                                                      \
    c = (inchar & 0xffU);                                                      \
    if (inchar != c)                                                           \
      break;                                                                   \
  }
/* Copy a chunk to fp_out */
#define copy_a_chunk(b1, b2, b3, b4)                                           \
  {                                                                            \
    if (buf[4] == b1 && buf[5] == b2 && buf[6] == b3 && buf[7] == b4) {        \
      for (i = 0; i < 8; i++)                                                  \
        putc(buf[i], fp_out);                                                  \
      for (i = 8; i < length + 12; i++) {                                      \
        GETBREAK;                                                              \
        putc(c, fp_out);                                                       \
      }                                                                        \
      continue;                                                                \
    }                                                                          \
  }
int png2protobuf(const char *in_file, const char *out_file) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  PngProto png_proto;

  unsigned char buf[MAX_CHUNK_LENGTH];
  unsigned char c;
  int inchar;
  unsigned int i;

  FILE *fp_in;
  if ((fp_in = fopen(in_file, "rb")) == NULL)
    return -1;

  /* Skip the 8-byte signature */
  // std::vector<uint8_t> png_signature_vec;
  for (i = 0; i < 8; i++) {
    GETBREAK;
    // png_signature_vec.push_back(c);
  }

  /* Copy chunks to png_proto */
  for (;;) {
    /* Read chunk length
     * NOTE LibPNG uses big-endian numbers!
     */
    uint32_t length; /* must be 32 bits! */
    GETBREAK;
    buf[0] = c;
    length = c;
    length <<= 8;
    GETBREAK;
    buf[1] = c;
    length += c;
    length <<= 8;
    GETBREAK;
    buf[2] = c;
    length += c;
    length <<= 8;
    GETBREAK;
    buf[3] = c;
    length += c;
    printf("\nlength:%d", length);

    /* Read chunk typename */
    GETBREAK;
    buf[4] = c;
    GETBREAK;
    buf[5] = c;
    GETBREAK;
    buf[6] = c;
    GETBREAK;
    buf[7] = c;
    printf("\ntype:%c%c%c%c", buf[4], buf[5], buf[6], buf[7]);

    /* Copy data bytes, but skip crc bytes */
    // Skip the IEND chunk
    if (buf[4] == 'I' && buf[5] == 'E' && buf[6] == 'N' && buf[7] == 'D') { // IEND
      break;
    }
    // IHDR chunk
    if (buf[4] == 'I' && buf[5] == 'H' && buf[6] == 'D' && buf[7] == 'R') { // IHDR
      uint32_t width;    /* must be 32 bits! */
      GETBREAK;
      width = c;
      width <<= 8;
      GETBREAK;
      width += c;
      width <<= 8;
      GETBREAK;
      width += c;
      width <<= 8;
      GETBREAK;
      width += c;

      uint32_t height; /* must be 32 bits! */
      GETBREAK;
      height = c;
      height <<= 8;
      GETBREAK;
      height += c;
      height <<= 8;
      GETBREAK;
      height += c;
      height <<= 8;
      GETBREAK;
      height += c;

      uint32_t other1; /* must be 32 bits! */
      GETBREAK;
      other1 = c;
      other1 <<= 8;
      GETBREAK;
      other1 += c;
      other1 <<= 8;
      GETBREAK;
      other1 += c;
      other1 <<= 8;
      GETBREAK;
      other1 += c;

      uint8_t other2; /* must be 8 bits! */
      GETBREAK;
      other2 = c;

      IHDR *ihdr = png_proto.mutable_ihdr();
      ihdr->set_width(width);
      ihdr->set_height(height);
      ihdr->set_other1(other1);
      ihdr->set_other2(other2);

      std::cout << std::endl;
      std::cout << "  width =" << ihdr->width() << std::endl;
      std::cout << "  height=" << ihdr->height() << std::endl;
      std::cout << "  other1=" << ihdr->other1() << std::endl;
      std::cout << "  other2=" << ihdr->other2() << std::endl;

      // skip crc
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    }

    PngChunk *png_chunk = png_proto.add_chunks();
    // PLTE chunk
    if (buf[4] == 'P' && buf[5] == 'L' && buf[6] == 'T' && buf[7] == 'E') {
      std::vector<uint8_t> plte_data;
      // Copy data bytes
      for (i = 8; i < length + 8; i++) {
        GETBREAK;
        plte_data.push_back(c);
      }

      PLTE *plte = png_chunk->mutable_plte();
      plte->set_data(plte_data.data(), plte_data.size());

      std::cout << "\ndata:\n  ";
      for (const auto e : plte_data)
        printf("%x ", e);
      printf("\n");

      // skip crc bytes
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    }
    // IDAT chunk
    if (buf[4] == 'I' && buf[5] == 'D' && buf[6] == 'A' && buf[7] == 'T') {
      std::vector<uint8_t> idat_data;
      // Copy data bytes
      for (i = 8; i < length + 8; i++) {
        GETBREAK;
        idat_data.push_back(c);
      }

      IDAT *idat = png_chunk->mutable_idat();
      idat->set_data(idat_data.data(), idat_data.size());

      std::cout << "\ndata:\n  ";
      for (const auto e : idat_data)
        printf("%x ", e);
      printf("\n");

      // Skip crc bytes
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    }
    // iCCP chunk
    if (buf[4] == 'i' && buf[5] == 'C' && buf[6] == 'C' && buf[7] == 'P') {
      std::vector<uint8_t> iccp_data;
      // Copy data bytes
      for (i = 8; i < length + 8; i++) {
        GETBREAK;
        iccp_data.push_back(c);
      }

      iCCP *iccp = png_chunk->mutable_iccp();
      iccp->set_data(iccp_data.data(), iccp_data.size());

      std::cout << "\ndata:\n  ";
      for (const auto e : iccp_data)
        printf("%x ", e);
      printf("\n");

      // Skip crc bytes
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    }

    // Other chunks: known or unknown
    if ( // known chunk types
        (buf[4] == 'b' && buf[5] == 'K' && buf[6] == 'G' && buf[7] == 'D') || // bKGD
        (buf[4] == 'c' && buf[5] == 'H' && buf[6] == 'R' && buf[7] == 'M') || // cHRM
        (buf[4] == 'd' && buf[5] == 'S' && buf[6] == 'I' && buf[7] == 'G') || // dSIG
        (buf[4] == 'e' && buf[5] == 'X' && buf[6] == 'I' && buf[7] == 'f') || // eXIf
        (buf[4] == 'g' && buf[5] == 'A' && buf[6] == 'M' && buf[7] == 'A') || // gAMA
        (buf[4] == 'h' && buf[5] == 'I' && buf[6] == 'S' && buf[7] == 'T') || // hIST
        (buf[4] == 'i' && buf[5] == 'T' && buf[6] == 'X' && buf[7] == 't') || // iTXt
        (buf[4] == 'p' && buf[5] == 'H' && buf[6] == 'Y' && buf[7] == 's') || // pHYs
        (buf[4] == 's' && buf[5] == 'B' && buf[6] == 'I' && buf[7] == 'T') || // sBIT
        (buf[4] == 's' && buf[5] == 'P' && buf[6] == 'L' && buf[7] == 'T') || // sPLT
        (buf[4] == 's' && buf[5] == 'R' && buf[6] == 'G' && buf[7] == 'B') || // sRGB
        (buf[4] == 's' && buf[5] == 'T' && buf[6] == 'E' && buf[7] == 'R') || // sTER
        (buf[4] == 't' && buf[5] == 'E' && buf[6] == 'X' && buf[7] == 't') || // tEXt
        (buf[4] == 't' && buf[5] == 'I' && buf[6] == 'M' && buf[7] == 'E') || // tIME
        (buf[4] == 't' && buf[5] == 'R' && buf[6] == 'N' && buf[7] == 'S') || // tRNS
        (buf[4] == 'z' && buf[5] == 'T' && buf[6] == 'X' && buf[7] == 't') || // zTXt
        (buf[4] == 's' && buf[5] == 'C' && buf[6] == 'A' && buf[7] == 'L') || // sCAL
        (buf[4] == 'p' && buf[5] == 'C' && buf[6] == 'A' && buf[7] == 'L') || // pCAL
        (buf[4] == 'o' && buf[5] == 'F' && buf[6] == 'F' && buf[7] == 's') // oFFs
    ) {
      std::vector<uint8_t> other_data;

      uint32_t type; /* must be 32 bits! */
      type = buf[4];
      type <<= 8;
      type += buf[5];
      type <<= 8;
      type += buf[6];
      type <<= 8;
      type += buf[7];

      // Copy data bytes
      for (i = 8; i < length + 8; i++) {
        GETBREAK;
        other_data.push_back(c);
      }

      OtherChunk *other = png_chunk->mutable_other_chunk();
      other->set_known_type(type);
      other->set_data(other_data.data(), other_data.size());

      std::cout << "\ndata:\n  ";
      for (const auto e : other_data)
        printf("%x ", e);
      printf("\n");

      // Skip crc bytes
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    } else { // unknown chunks
      std::vector<uint8_t> other_data;

      uint32_t type; /* must be 32 bits! */
      type = buf[4];
      type <<= 8;
      type += buf[5];
      type <<= 8;
      type += buf[6];
      type <<= 8;
      type += buf[7];

      // Copy data bytes
      for (i = 8; i < length + 8; i++) {
        GETBREAK;
        other_data.push_back(c);
      }

      OtherChunk *other = png_chunk->mutable_other_chunk();
      other->set_unknown_type(type);
      other->set_data(other_data.data(), other_data.size());

      std::cout << "\ndata:\n  ";
      for (const auto e : other_data)
        printf("%x ", e);
      printf("\n");

      // Skip crc bytes
      for (i = 0; i < 4; i++) {
        GETBREAK;
      }
      continue;
    }

    if (inchar != c)
      break; // EOF
  }          // end of "for"
  printf("\n");

  // Write to disk
  std::fstream output(out_file,
                      std::ios::out | std::ios::trunc | std::ios::binary);
  if (!png_proto.SerializeToOstream(&output)) {
    std::cerr << "Writing to disk failed!" << std::endl;
  }

  output.close();
  fclose(fp_in);
  // Optional: Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}

/* Convert protobuf file to png file
 * @in_file: protobuf file
 * @out_file: png file
 */
int protobuf2png(const char *in_file, const char *out_file) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  PngProto png_proto;
  std::stringstream all;

  // Parse the protobuf file
  std::fstream input(in_file, std::ios::in | std::ios::binary);
  if (!png_proto.ParseFromIstream(&input)) {
    std::cerr << "Failed to parse the protobuf file!" << std::endl;
    return -1;
  }

  /* Simplest PNG file:        PNG Signature + IHDR + IDATA + IEND
   * Second Simplest PNG file: PNG Signature + IHDR + PLTE + IDATA + IEND
   */
  // I: PNG Signature (the first 8 bytes of a PNG file)
  const unsigned char signature[] = {0x89, 0x50, 0x4e, 0x47,
                                     0x0d, 0x0a, 0x1a, 0x0a};
  all.write((const char *)signature, sizeof(signature));

  /* II: IHDR (the image header chunk)
   *
   * IHDR must be the first chunk in a PNG image, and it includes all of the
   * details about the type of the image: its height and width, pixel depth,
   * compression and filtering methods, interlacing method, whether it has an
   * alpha (transparency) channel, and whether it's a truecolor, grayscale, or
   * colormapped (palette) image.
   */
  std::stringstream ihdr_str;
  if (png_proto.has_ihdr()) {
    auto &ihdr = png_proto.ihdr();
    std::cout << std::endl
              << "IHDR:\n"
              << "  width  = " << ihdr.width() << "\n"
              << "  height = " << ihdr.height() << "\n"
              << "  other1 = " << ihdr.other1() << "\n"
              << "  other2 = " << ihdr.other2() << "\n";

    WriteInt(ihdr_str, ihdr.width());
    WriteInt(ihdr_str, ihdr.height());
    WriteInt(ihdr_str, ihdr.other1());
    WriteByte(ihdr_str, ihdr.other2());

    WriteChunk(all, "IHDR", ihdr_str.str());
  }

  /* III: PLTE (the palette chunk) or IDAT (the image data chunk) or ...
   *
   * IDAT contains all of the image's compressed pixel data. Although single
   * IDATs are perfectly valid as long as they contain no more than 2 gigabytes
   * of compressed data, in most images the compressed data is split into
   * several IDAT chunks for greater robustness. Small IDAT chunks are by far
   * the most common, particularly in sizes of 8 or 32 KB.
   *
   * IHDR, IDAT and IDEND chunk types are sufficient to build truecolor and
   * grayscale PNG files, with or without an alpha channel, but palette-based
   * images require one more: PLTE, the palette chunk.
   *
   * PLTE simply contains a sequence of red, green, and blue values, where a
   * value of 0 is black and 255 is full intensity; anywhere from 1 to 256 RGB
   * triplets are allowed, depending on the pixel depth of the image. (That is,
   * for a 4-bit image, no more than 16 palette entries are allowed.)
   *
   * The PLTE chunk must come before the first IDAT chunk.
   */
  for (size_t i = 0, n = png_proto.chunks_size(); i < n; i++) {
    auto &chunk = png_proto.chunks(i);

    if (chunk.has_plte()) {
      WriteChunk(all, "PLTE", chunk.plte().data());
    } else if (chunk.has_idat()) { // NOTE the compress parameter
      // WriteChunk(all, "IDAT", chunk.idat().data(), true);
      WriteChunk(all, "IDAT", chunk.idat().data()); // defaults to "false"
    } else if (chunk.has_iccp()) {
      WriteChunk(all, "iCCP", chunk.iccp().data());
    } else if (chunk.has_other_chunk()) {
      auto &other_chunk = chunk.other_chunk();
      char type[5] = {0};

      uint32_t type_int;
      if (other_chunk.has_known_type()) {
        type_int = other_chunk.known_type();
      } else if (other_chunk.has_unknown_type()) {
        type_int = other_chunk.unknown_type();
      } else {
        continue;
      }

      /* NOTE the function here!
       * LibPNG uses big-endian numbers!
       */
      type_int = __builtin_bswap32(type_int);
      memcpy(type, &type_int, 4);

      WriteChunk(all, type, other_chunk.data());
    }
  } // end of "for"

  /* IV: IEND (the end-of-image chunk)
   *
   * IEND is the simplest chunk of all; it contains no data, just indicates that
   * there are no more chunks in the image. And it serves as one more check that
   * the PNG file is complete and internally self-consistent.
   */
  WriteChunk(all, "IEND", "");

  std::string res = all.str();
  std::ofstream of(out_file);
  of.write(res.data(), res.size());

  input.close();
  // Optional: Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return 0;
}
