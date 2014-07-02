#include "common.hpp"

FILE *Fopen(const char *filename, const char *mode) {
  FILE *fp = fopen(filename, mode);
  if (fp == NULL) {
    fprintf(stderr, "Problem opening file %s\n", filename);
    exit(0);
  }
  return fp;
}


int CheckFile(const char *filename, const char *mode) {
  FILE *fp = fopen(filename, mode);
  if (fp == NULL) {
      fprintf(stderr, 
              "      Problem opening file %s\n", filename);
      return 0;
  }
  fclose(fp);
  return 1;
}


void print_array(string msg, vector<string> v) {
  fprintf(stderr, "      %s", msg.c_str());
  for (size_t i = 0; i < v.size(); i++) {
    if (i > 0) fprintf(stderr, ", ");
    fprintf(stderr, "%s", v[i].c_str());
  }
  fprintf(stderr, "]\n");
}
