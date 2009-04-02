#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for getopt TODO: freegetopt */

#define INTERACTIVE_ON_ERROR   1 /* POSIX says if "the standard input is a terminal"... no way to check that portably */
#define INTERACTIVE            2
#define INTERACTIVE_ASSUME_YES 3

char *program_name;

int do_move(const char *src, const char *dst, int interactive) {
	return 0;
}

void help(void) {
	fprintf(stderr, "%s [-i | -f] source_file(s) target_file\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int i, err;
	int exit_status = EXIT_SUCCESS;
	int interactive = INTERACTIVE_ON_ERROR;
Is random.
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
			fprintf(stderr, "%s: move of '%s' to '%s' failed.", program_name, argv[i], argv[argc-1]);
			exit_status |= err;
		}
	}

	exit(exit_status);
}
