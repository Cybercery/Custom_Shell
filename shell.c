#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <pwd.h> // for getting home directory

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define HISTORY_FILE ".customshell_history"

#define SHELL_MAX_INPUT 1024
#define MAX_ARGS 64


// listing built in commands
void shellp()
{
	printf("Built-ins:\n");
	printf("cd <dir> - change current directory\n");
	printf("help     - therapy\n");
	printf("exit	 - probably exit idk\n");
	printf("textedit - create/edit textfile");
	printf("banner   - display ascii art from image file\n");
	printf("Use '&' at the end of your command to run it in the background. \n");
	printf("External commands run via execvp().\n");
}
void show_saved_banner() {
    FILE *fp = fopen(".banner.txt", "r");
    if (!fp) return; // no saved banner yet
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
    printf("\n");
}

// ascii art from image
void print_image_ascii(const char *filename, int width, const char *save_path) {
    int x, y, n;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 1);
    if (!data) {
        printf("Error: could not load image '%s'\n", filename);
        return;
    }

    int new_width = width;
    int new_height = (y * width) / (2 * x); // maintain aspect ratio
    const char *ascii = " .:-=+*#%@"; // brightness map

    FILE *out = stdout; // default = print to terminal
    if (save_path) {
        out = fopen(save_path, "w");
        if (!out) {
            printf("Error: could not open '%s' for writing\n", save_path);
            stbi_image_free(data);
            return;
        }
    }

    for (int j = 0; j < new_height; j++) {
        for (int i = 0; i < new_width; i++) {
            int src_x = i * x / new_width;
            int src_y = j * y / new_height;
            unsigned char pixel = data[src_y * x + src_x];
            int index = (pixel * 9) / 255;
            fprintf(out, "%c", ascii[index]);
        }
        fprintf(out, "\n");
    }

    if (save_path) {
        fclose(out);
        printf("Saved ASCII art to %s\n", save_path);
    }

    stbi_image_free(data);
}


int shell_cd(char **args)
{
	if(args[1] == NULL)
	{
		fprintf(stderr, "CustomShell: cd:missing arguemnts\n"); //errors for directory stuff
		return -1;
	}

	if(chdir(args[1]) != 0)
	{
		perror("CustomShell: cd");
	}
	return 0;
}

//splitting input lines into tokens
void parse_input(char *input, char **args)
{
	for(int i = 0; i< MAX_ARGS; i++)
	{
		args[i] = strsep(&input, " ");
		if(args[i] == NULL) break;
		if(strlen(args[i]) == 0) i--; //skipping empty tokens
	}
}

// child death handling
void handle_child_exit(int sig)
{
	int status;
	pid_t pid;
	(void)sig;//unused

	//reap all finished children
	while((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		if(WIFEXITED(status))
		{
			printf("\n[done] PID %d exited with status %d\n", pid, WEXITSTATUS(status));
		}
		else if (WIFSIGNALED(status))
		{
			printf("\n[done] PID %d killed by signal %d\n", pid, WTERMSIG(status));
		}
		fflush(stdout);
	}
}

int main()
{
	char input[SHELL_MAX_INPUT];
	char *args[MAX_ARGS];
	pid_t pid;
	int status;

	// checks if background processes are finished
	signal(SIGCHLD, handle_child_exit);


	show_saved_banner();

	while(1)
	{

	char cwd[256];
	if(getcwd(cwd, sizeof(cwd)) != NULL)
		printf("CustomShell: %s> ", cwd);
	else
		printf("CustomShell> ");


	fflush(stdout);

	// reading commands from  user
	if (fgets(input, sizeof(input), stdin) == NULL)
		break;

	input[strcspn(input, "\n")] = '\0'; //removing newline

	parse_input(input, args);


	// empty input check
	if (args[0] == NULL)
		continue;

	// hanndling  built in commands

	if (strcmp(input, "exit") == 0)
	break; //exit command

	else if (strcmp(args[0], "cd") == 0)
	{
		shell_cd(args);
		continue;
	}
	else if(strcmp(args[0], "help") == 0)
	{
		shellp();
	}
	// changing banner 
	else if (strcmp(args[0], "banner") == 0) {
    if (args[1] == NULL) {
        printf("Usage:\n");
        printf("  banner <image>       - show image as ASCII\n");
        printf("  banner set <image>   - save image for next startup\n");
        printf("  banner clear         - remove saved banner\n");
        continue;
    }

    if (strcmp(args[1], "set") == 0 && args[2]) {
        print_image_ascii(args[2], 60, ".banner.txt");
        printf("Saved banner for next startup!\n");
    } else if (strcmp(args[1], "clear") == 0) {
        remove(".banner.txt");
        printf("Banner cleared.\n");
    } else {
        print_image_ascii(args[1], 60, NULL);
    }

    continue;
	}



	// textedit
	if(strcmp(args[0], "textedit") == 0)
	{
		if(args[1] == NULL)
		{
			printf("Usage: textedit <filename>\n");
			return 1;
		}
		pid_t pid = fork();
		if (pid == 0)
		{
			char *editor_args[] = {"./textedit", args[1], NULL};
			execvp(editor_args[0], editor_args);
			perror("execvp");
			exit(1);
		}
		else if(pid > 0)
		{
			int status;
			waitpid(pid, &status, 0);
		}
		else
		{
			perror("fork");
		}
		continue;
	}

	// background exec check

	int run_in_background = 0;
	int last = 0;

	for(int i = 0; args[i] != NULL; i++) last = i;

	if(strcmp(args[last], "&") == 0)
	{
		run_in_background = 1;
		args[last] = NULL;
	}

	// external commands
	pid = fork();

	if (pid == 0)
	{
		// child process
		if(execvp(args[0], args) == -1)
			perror("exec failed");
		exit(1);
	}
	else if (pid > 0)
	{	//parent
		if(!run_in_background)
		{
			waitpid(pid, &status, 0);
		}
		else
		{
			printf("[background] started PID %d\n", pid);
		}
	}
	else
	{
		perror("fork failed");
	}

	}

	printf("macos is bad stop using it\n");
	return 0;
}
