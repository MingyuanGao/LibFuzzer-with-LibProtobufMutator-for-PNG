#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>  // for crc32

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <functional>

/* Normally use <png.h> here to get the installed libpng, but this is done to
 * ensure the code picks up the local libpng implementation!
 */
#include "libpng/png.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "png.pb.h"

static void WriteInt(std::stringstream &out, uint32_t x);
static void WriteByte(std::stringstream &out, uint8_t x);
static std::string Compress(const std::string &s);
static void WriteChunk(std::stringstream &out, const char *type, const std::string &chunk, bool compress = false);
std::string Proto2PNG(const PngProto &png_proto);

/////////////////////////////////////////////////////////////////
// The actual fuzz target that consumes the protobuf data.
extern "C" int FuzzPNG(const uint8_t* data, size_t size);

/* NOTE Use the correct macro, i.e., DEFINE_PROTO_FUZZER or DEFINE_BINARY_PROTO_FUZZER
 * 
 * For example, to use the PNG seeds in "seeds-protobuf" dir, use the 
 * "DEFINE_BINARY_PROTO_FUZZER" macro.
 */
DEFINE_PROTO_FUZZER(const PngProto &png_proto) {
  auto s = Proto2PNG(png_proto);

  //std::size_t png_proto_hash = std::hash<std::string>{}( png_proto.SerializeAsString() );
  //std::string dump_path = std::to_string(png_proto_hash) + ".png";
  //std::ofstream of(dump_path);
  //of.write(s.data(), s.size());
  //of.close();

  FuzzPNG((const uint8_t*)s.data(), s.size());
}
/////////////////////////////////////////////////////////////////

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
static void WriteChunk(std::stringstream &out, const char *type, const std::string &chunk, bool compress) {
  std::string compressed;
  const std::string *s = &chunk;
  if (compress) {
    compressed = Compress(chunk);
    s = &compressed;
  }
  uint32_t len = s->size();
  uint32_t crc = crc32(crc32(0, (const unsigned char *)type, 4), (const unsigned char *)s->data(), s->size());
 
  WriteInt(out, len); // length
  out.write(type, 4); // type
  out.write(s->data(), s->size()); // data
  WriteInt(out, crc); // crc
}

// The "Protobuf to PNG converter" for the fuzzer 
std::string Proto2PNG(const PngProto &png_proto) {
  std::stringstream all;
 
  /* Simplest PNG file:        PNG Signature + IHDR + IDATA + IEND
   * Second Simplest PNG file: PNG Signature + IHDR + PLTE + IDATA + IEND
   */
  
  // I: PNG Signature (the first 8 bytes of a PNG file) 
  const unsigned char signature[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
  all.write((const char*)signature, sizeof(signature));
	
  /* II: IHDR (the image header chunk)
   * 
   * IHDR must be the first chunk in a PNG image, and it includes all of the details 
   * about the type of the image: its height and width, pixel depth, compression and 
   * filtering methods, interlacing method, whether it has an alpha (transparency) 
   * channel, and whether it's a truecolor, grayscale, or colormapped (palette) image.
   */
  std::stringstream ihdr_str;
  auto &ihdr = png_proto.ihdr();
  // Avoid large images.
  // They may have interesting bugs, but OOMs are going to kill fuzzing.
  uint32_t w = std::min(ihdr.width(), 4096U);
  uint32_t h = std::min(ihdr.height(), 4096U);
  WriteInt(ihdr_str, w);
  WriteInt(ihdr_str, h);
  WriteInt(ihdr_str, ihdr.other1());
  WriteByte(ihdr_str, ihdr.other2());
  WriteChunk(all, "IHDR", ihdr_str.str());

  /* III: PLTE (the palette chunk) or IDAT (the image data chunk) or ...
   *  
   * IDAT contains all of the image's compressed pixel data. Although single IDATs are 
   * perfectly valid as long as they contain no more than 2 gigabytes of compressed 
   * data, in most images the compressed data is split into several IDAT chunks for 
   * greater robustness. Small IDAT chunks are by far the most common, particularly in 
   * sizes of 8 or 32 KB.
   *
   * IHDR, IDAT and IDEND chunk types are sufficient to build truecolor and grayscale 
   * PNG files, with or without an alpha channel, but palette-based images require one 
   * more: PLTE, the palette chunk. 
   *
   * PLTE simply contains a sequence of red, green, and blue values, where a value of 
   * 0 is black and 255 is full intensity; anywhere from 1 to 256 RGB triplets are 
   * allowed, depending on the pixel depth of the image. (That is, for a 4-bit image, 
   * no more than 16 palette entries are allowed.) 
   *
   * The PLTE chunk must come before the first IDAT chunk.
   */
  for (size_t i = 0, n = png_proto.chunks_size(); i < n; i++) {
    auto &chunk = png_proto.chunks(i);
    if (chunk.has_plte()) {
      WriteChunk(all, "PLTE", chunk.plte().data());
    } else if (chunk.has_idat()) {
      WriteChunk(all, "IDAT", chunk.idat().data(), true); // TODO: true or false?
    } else if (chunk.has_iccp()) {
      std::stringstream iccp_str;
      iccp_str << "xyz";  // don't fuzz iCCP name field.
      WriteByte(iccp_str, 0);
      WriteByte(iccp_str, 0);
      auto compressed_data = Compress(chunk.iccp().data());
      iccp_str.write(compressed_data.data(), compressed_data.size());
      WriteChunk(all, "iCCP", iccp_str.str());
    } else if (chunk.has_other_chunk()) {
      auto &other_chunk = chunk.other_chunk();
      char type[5] = {0};
      if (other_chunk.has_known_type()) {
        static const char * known_chunks[] = {
            "bKGD", "cHRM", "dSIG", "eXIf", "gAMA", "hIST", "iCCP",
            "iTXt", "pHYs", "sBIT", "sPLT", "sRGB", "sTER", "tEXt",
            "tIME", "tRNS", "zTXt", "sCAL", "pCAL", "oFFs"
        };
        size_t known_chunks_size = sizeof(known_chunks) / sizeof(known_chunks[0]);
        size_t chunk_idx = other_chunk.known_type() % known_chunks_size;
        memcpy(type, known_chunks[chunk_idx], 4);
      } else if (other_chunk.has_unknown_type()) {
        uint32_t unknown_type_int = other_chunk.unknown_type();
        memcpy(type, &unknown_type_int, 4);
      } else {
        continue;
      }
      type[4] = 0;
      WriteChunk(all, type, other_chunk.data());
    }
  }
  
  /* IV: IEND (the end-of-image chunk)
   * 
   * IEND is the simplest chunk of all; it contains no data, just indicates that 
   * there are no more chunks in the image. And it serves as one more check that 
   * the PNG file is complete and internally self-consistent.
   */
  WriteChunk(all, "IEND", "");
  std::string res = all.str();
 
  // if (const char *dump_path = getenv("PROTO_FUZZER_DUMP_PATH")) {
  //   // With libFuzzer binary run this to generate a PNG file x.png:
  //   // PROTO_FUZZER_DUMP_PATH=x.png ./a.out proto-input
  //   std::ofstream of(dump_path);
  //   of.write(res.data(), res.size());
  // }
  
  return res;
}
