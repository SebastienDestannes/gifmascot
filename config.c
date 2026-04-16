#include "gifmascot.h"

t_config LoadConfigFile(Display *dpy, uint32_t gif_w, uint32_t gif_h) {
  char line[256];
  t_config conf = {0, 0, 0.01, ""};
  char *home = getenv("HOME");
  char *filepath = "/.config/gifpet.conf";
  strncpy(conf.path, home, 50);
  strncat(conf.path, filepath, 50);
  FILE *fp = fopen(conf.path, "r");
  if (!fp) {
    // Pas de config sauvegardée : se placer sur le terminal actif
    Window term = GetParentTerminalWindow(dpy);
    if (term) {
      unsigned int tw, th;
      t_coord pos = GetFrameGeometry(dpy, term, &tw, &th);    
      int scale = 2;
      conf.x = (pos.x / scale) + (int)(tw / scale) - (int)gif_w / 2;
      conf.y = (pos.y / scale) + 23 - (int)gif_h; // 23 c'est la taille de la barre d'outil
    }
    // Créer le dossier si besoin et sauvegarder immédiatement
    t_coord coord = {conf.x, conf.y};
    UpdateConfigFile(conf, coord);  // ← écrire dès le premier lancement
    return conf;
  }
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
  sprintf(buf, "%d\n", coord.x / 2);
  fputs(buf, fp);
  sprintf(buf, "%d\n", coord.y / 2);
  fputs(buf, fp);
  fclose(fp);
  printf("Updated config!\n");
}