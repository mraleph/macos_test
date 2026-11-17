#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

static void write_script(const char* path, const char* msg) {
  FILE* f = fopen(path, "w");
  fprintf(f, "#!/bin/sh\n"
    "MY_PATH=\"${BASH_SOURCE[0]}\"\n"
    "echo \"  -> ran ($MY_PATH) [$1] %s in $(pwd -P)\"\n", msg);
  fclose(f);
  chmod(path, 0755);
}

static void try_spawn(const char* label, int use_p, const char* exe, const char* wd, const char* msg) {
  posix_spawn_file_actions_t fa;
  posix_spawn_file_actions_init(&fa);
  posix_spawn_file_actions_addchdir_np(&fa, wd);
  char* argv[] = {(char*)exe, msg, NULL};
  pid_t pid;
  fflush(stdout);
  int rc = use_p ? posix_spawnp(&pid, exe, &fa, NULL, argv, environ)
                 : posix_spawn(&pid, exe, &fa, NULL, argv, environ);
  if (rc != 0) {
    printf("%s: rc=%d (%s)\n", label, rc, strerror(rc));
  } else {
    int st;
    waitpid(pid, &st, 0);
    printf("%s: rc=0, exit=%d\n", label, WEXITSTATUS(st));
  }
  fflush(stdout);
  posix_spawn_file_actions_destroy(&fa);
}

int main(void) {
  char root[] = "/tmp/spawnchdirXXXXXX";
  mkdtemp(root);
  char work[512], tool[512], decoy[512];
  snprintf(work, sizeof work, "%s/work", root);
  mkdir(work, 0755);
  snprintf(tool, sizeof tool, "%s/tool.sh", work);
  write_script(tool, "WORK/tool.sh");
  chdir(root);

  try_spawn("posix_spawnp ./tool.sh", 1, "./tool.sh", work, "v1");
  puts("\n\n");
  try_spawn("posix_spawn  ./tool.sh", 0, "./tool.sh", work, "v2");
  puts("\n\n");

  snprintf(decoy, sizeof decoy, "%s/tool.sh", root);
  write_script(decoy, "PARENT-CWD/tool.sh (decoy)");
  printf("-- with a decoy ./tool.sh in the parent cwd:\n");
  try_spawn("posix_spawnp ./tool.sh", 1, "./tool.sh", work, "v3");
  puts("\n\n");
  try_spawn("posix_spawn  ./tool.sh", 0, "./tool.sh", work, "v4");
  puts("\n\n");
  return 0;
}