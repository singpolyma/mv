#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(__unix__)
	#include <unistd.h>
#else
	#include "getopt.h"
#endif

#define INTERACTIVE_ASSUME_YES 1
#define INTERACTIVE_ON_ERROR   2
#define INTERACTIVE            3

#define RENAME_ON_REBOOT 110

#if defined(_WIN32)
	#include <io.h>
	#define TRY_BACKSLASH_AND_SLASH(p) \
	       	name = strrchr((p), '\\'); \
		if(!name) { \
			name = strrchr((p), '/'); \
		} \
		if(!name) { \
			return (p); \
		}
	char *basename(char *path) {
		char *name;
		if(path == NULL || path[0] == '\0') {
			return ".";
		}
		if(strlen(path) == 1 && (path[0] == '\\' || path[0] == '/')) {
			return "/";
		}
		TRY_BACKSLASH_AND_SLASH(path);
		if(name[1] == '\0') {
			*name = '\0';
			TRY_BACKSLASH_AND_SLASH(path);
		}
		if(name[1] == '\0') {
			return "/";
		}
		return name+1;
	}
#endif
#if defined(__unix__)
	#include <libgen.h>
#endif
#if defined(_WIN32) || defined(__unix__)
	#include <sys/stat.h>
	int isdir(const char *path) {
		struct stat st;
		return (!stat(path, &st) && S_ISDIR(st.st_mode));
	}
	char *build_dst(const char *src, const char *dst) {
		if(isdir(dst)) {
			char *src_copy = strdup(src);
			char *path = strdup(dst);
			char *file = basename(src_copy);
			if(!path || !file) return NULL;
			path = realloc(path, strlen(path) + strlen(file) + 2);
			if(!path) return NULL;
			if(path[strlen(path)-1] != '/') {
				strcat(path, "/");
			}
			strcat(path, file);
			free(src_copy);
			return path;
		} else {
			return strdup(dst);
		}
	}
	int file_exists(const char *dst) {
		FILE *fp;
		fp = fopen(dst, "rb");
		if(fp) { /* The file exists */
			fclose(fp);
			return 1;
		}
		return 0;
	}
#endif

int do_move(const char *src, const char *dst, int interactive) {
	char *built_dst = NULL;
	int err = 0;
	#if defined(_WIN32) || defined(__unix__)
		built_dst = build_dst(src, dst);
		if(!built_dst) {
			perror("do_move call to build_dst failed");
			exit(EXIT_FAILURE);
		}
		/* POSIX says the default depends on if stdin isatty. */
		if(interactive == INTERACTIVE_ON_ERROR && !isatty(fileno(stdin))) {
			interactive = INTERACTIVE_ASSUME_YES;
		}
	#endif
	/* If running in interactive mode, prompt for overwrite. */
	if(interactive > INTERACTIVE_ASSUME_YES) {
	#if defined(_WIN32) || defined(__unix__)
		if(file_exists(built_dst)) {
			int yesno;
			fprintf(stderr, "'%s' already exists. Overwrite? [Yn] ", built_dst);
			if((yesno = fgetc(stdin)) == EOF) {
				fputs("Reading from STDIN failed.\n", stderr);
				return EXIT_FAILURE;
			}
			if(yesno == 'n' || yesno == 'N') {
				return 0;
			}
		}
	#else
		/* C89 gives us no way to construct the path, so just always prompt. */
		char yesno;
		fprintf(stderr, "Can't tell if '%s' exists. Move anyway? [Yn] ", dst);
		if((yesno = fgets(stdin)) == EOF) {
			fputs("Reading from STDIN failed.\n", stderr);
			return EXIT_FAILURE;
		}
		if(yesno == 'n' || yesno == 'N') {
			return 0;
		}
	#endif
	}
	/* Try to move */
	#if defined(_WIN32)
		/* if(interactive > INERACTIVE_ASSUME_YES && dst is in use) prompt for rename-at-reboot return EXIT_FAILURE on no, RENAME_ON_REBOOT on yes. XXX: Not in POSIX */
		/* if interactive == INTERACTIVE_ASSUME_YES && dst is in use) rename-at reboot. return RENAME_ON_REBOOT. XXX: Not in POSIX */
		/* TODO */
	#else
		if(built_dst) {
			err = rename(src, built_dst);
		} else {
			err = rename(src, dst);
		}
		if(err) {
			err = errno;
		}
	#endif
	if(built_dst) {
		free(built_dst);
	}
	return err;
}

void help(void) {
	fprintf(stderr, "%s [-i | -f] source_file(s) target_file\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int i, err;
	int exit_status = EXIT_SUCCESS;
	int interactive = INTERACTIVE_ON_ERROR;
	program_name = argv[0];

	while((i = getopt(argc, argv, "if")) != -1) {
		switch(i) {
			case 'i':
				interactive = INTERACTIVE;
				break;
			case 'f':
				interactive = INTERACTIVE_ASSUME_YES;
				break;
			default:
				help();
		}
	}
	argc -= optind;
	argv += optind;
	if(argc < 2) {
		help();
	}

	for(i = 0; i < argc-1; i++) {
		err = do_move(argv[i], argv[argc-1], interactive);
		if(err) {
			fprintf(stderr, "%s: move of '%s' to '%s' failed.\n", program_name, argv[i], argv[argc-1]);
			if(exit_status != RENAME_ON_REBOOT) {
				exit_status = err;
			}
		}
	}

	exit(exit_status);
}
