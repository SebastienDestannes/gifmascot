#include "gifmascot.h"

t_config LoadConfigFile() {
  char line[256];
  t_config conf = {0, 0, 0.01, ""};
  char *home = getenv("HOME");
  char *filepath = "/.config/gifpet.conf";
  strlcpy(conf.path, home, 50);
  strlcat(conf.path, filepath, 50);
  FILE *fp = fopen(conf.path, "r");
  if (!fp)
    return conf;
  fgets(line, 10, fp);
  conf.x = atoi(line);
  fgets(line, 10, fp);
  conf.y = atoi(line);
  fclose(fp);
  return conf;
}

void UpdateConfigFile(t_config conf, t_coord coord) {
  FILE *fp = fopen(conf.path, "w");
  char buf[50];
  if (!fp)
    return;
  sprintf(buf, "%d\n", coord.x);
  fputs(buf, fp);
  sprintf(buf, "%d\n", coord.y);
  fputs(buf, fp);
  fclose(fp);
  printf("Updated config!\n");
}
