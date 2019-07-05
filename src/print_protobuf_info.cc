#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "png.pb.h"

/* Print the png info from a protobuf file
 * @in_file: protobuf file
 */
int print_protobuf_info(const char *in_file) {
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  PngProto png_proto;

  // Parse the protobuf file
  std::fstream input(in_file, std::ios::in | std::ios::binary);
  if (!png_proto.ParseFromIstream(&input)) {
    std::cerr << "Failed to parse the protobuf file!" << std::endl;
    return -1;
  }

  if (png_proto.has_ihdr()) {
    auto &ihdr = png_proto.ihdr();
    std::cout << "type:IHDR\n"
              << "  width  = " << ihdr.width() << "\n"
              << "  height = " << ihdr.height() << "\n"
              << "  other1 = " << ihdr.other1() << "\n"
              << "  other2 = " << ihdr.other2() << "\n";

    std::cout << "\n\n";
  }

  for (size_t i = 0, n = png_proto.chunks_size(); i < n; i++) {
    auto &chunk = png_proto.chunks(i);

    if (chunk.has_plte()) {
      std::cout << "length:" << chunk.plte().data().size() << "\n";
      std::cout << "type:PLTE\n";
      std::cout << "data:";
      for (const uint8_t e : chunk.plte().data())
        printf("%x ", e);
      std::cout << "\n\n";
    } else if (chunk.has_idat()) {
      std::cout << "length:" << chunk.idat().data().size() << "\n";
      std::cout << "type:IDAT\n";
      std::cout << "data:";
      for (const uint8_t e : chunk.idat().data())
        printf("%x ", e);
      std::cout << "\n\n";
    } else if (chunk.has_iccp()) {
      std::cout << "length:" << chunk.iccp().data().size() << "\n";
      std::cout << "type:iCCP\n";
      std::cout << "data:";
      for (const uint8_t e : chunk.iccp().data())
        printf("%x ", e);
      std::cout << "\n\n";
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

      std::cout << "length:" << chunk.other_chunk().data().size() << "\n";

      type_int = __builtin_bswap32(type_int);
      memcpy(type, &type_int, 4);
      printf("type:%c%c%c%c\n", type[0], type[1], type[2], type[3]);

      std::cout << "data:";
      for (const uint8_t e : chunk.other_chunk().data())
        printf("%x ", e);
      std::cout << "\n\n";
    }
  } // end of "for"

  return 0;
}

int main(int argc, const char **argv) {
  if (argc == 2) {
    print_protobuf_info(argv[1]);
  } else { //  Wrong number of arguments
    fprintf(stderr, "usage: print_protobuf_info input-file\n");
  }

  return 0;
}
