#pragma once

#include <string>
#include "png.pb.h" 

static void WriteInt(std::stringstream &out, uint32_t x);
static void WriteByte(std::stringstream &out, uint8_t x);
static std::string Compress(const std::string &s);
static void WriteChunk(std::stringstream &out, const char *type, const std::string &chunk, bool compress = false);

int print_png_info (const char* in_file);
int png2png(const char* in_file, const char* out_file);
int png2protobuf (const char* in_file, const char* out_file);
int protobuf2png (const char* in_file, const char* out_file);
