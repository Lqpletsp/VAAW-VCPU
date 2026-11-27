#include <stdint.h>
#include <stdio.h>

enum isa : uint8_t {
  end,
  set_32b,
  cipr,
  add_s,
  add_u,
  min_s,
  min_u,
  mlt_s,
  mlt_u,
  division_s,
  division_u,
  logic_and,
  logic_or,
  logic_xor,
  logic_not,
};

int write_to_disk(FILE *fp) {
  uint8_t data[] = {(uint8_t)set_32b, 2, 10, 00, 00, 0, 1, 0, (uint8_t)end};
  size_t count = sizeof(data) / sizeof(data[0]);
  size_t written = fwrite(data, sizeof(data[0]), count, fp);
  if (written != count) {
    printf("error writing to file\n");
    return 1;
  }
  return 0;
}

int read_disk(FILE *fp) {
  if (fp == NULL) {
    return 1;
  }
  rewind(fp);
  uint8_t buffer[16] = {0};
  size_t bytes_read =
      fread(buffer, sizeof(buffer[0]), sizeof(buffer) / sizeof(buffer[0]), fp);
  for (size_t i = 0; i < bytes_read; ++i) {
    printf("%u  ", buffer[i]);
  }
  printf("\n");
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("invalid execution format\n");
    return 1;
  }
  FILE *file = fopen(argv[1], "r+b");
  if (file == NULL) {
    printf("failed to load the file\n");
    return 1;
  }
  if (write_to_disk(file) != 0) {
    fclose(file);
    return 1;
  }
  write_to_disk(file);
  read_disk(file);
  fclose(file);
  return 0;
}