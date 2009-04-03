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
	#include <windows.h>
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
	int file_exists(const char *dst) {
		HANDLE fp = CreateFile(dst, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		int err = GetLastError();
		CloseHandle(fp);
		if(err == ERROR_SUCCESS) {
			return 1;
		}
		if(err == ERROR_SHARING_VIOLATION) {
			return 2;
		}
		return 0;
	}
#endif
#if defined(__unix__)
	#include <libgen.h>
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
#endif

int do_move(const char *src, const char *dst, int interactive) {
	char *built_dst = NULL;
	int err = 0;
	int exists = 0;
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
		exists = file_exists(built_dst);
		if(exists) {
			int yesno;
			if(exists == 1) {
				fprintf(stderr, "'%s' already exists. Overwrite? [Yn] ", built_dst);
			} else if(exists == 2) {
				fprintf(stderr, "'%s' is in use. Overwrite on reboot? [Yn] ", built_dst);
			}
			fflush(stderr);
			if((yesno = fgetc(stdin)) == EOF) {
				fputs("Reading from STDIN failed.\n", stderr);
				return EXIT_FAILURE;
			}
			if(yesno == 'n' || yesno == 'N') {
				return exists == 2 ? EXIT_FAILURE : 0;
			}
		}
	#else
		/* C89 gives us no way to construct the path, so just always prompt. */
		char yesno;
		fprintf(stderr, "Can't tell if '%s' exists. Move anyway? [Yn] ", dst);
		fflush(stderr);
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
		if(interactive == INTERACTIVE_ASSUME_YES) {
			exists = file_exists(built_dst ? built_dst : dst);
		}
		if(exists == 2) { /* rename on reboot */
			/* TODO */
		} else { /* just rename */
			/* TODO */
		}
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

void help(char *program_name) {
	fprintf(stderr, "%s [-i | -f] source_file(s) target_file\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int i, err;
	int exit_status = EXIT_SUCCESS;
	int interactive = INTERACTIVE_ON_ERROR;
	char *program_name = argv[0];

	while((i = getopt(argc, argv, "if")) != -1) {
		switch(i) {
			case 'i':
				interactive = INTERACTIVE;
				break;
			case 'f':
				interactive = INTERACTIVE_ASSUME_YES;
				break;
			default:
				help(program_name);
		}
	}
	argc -= optind;
	argv += optind;
	if(argc < 2) {
		help(program_name);
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
