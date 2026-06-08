# CustomShell

A lightweight Unix shell written in C featuring a built-in terminal text editor and customizable ASCII art startup banners.

## Features

* Execute external commands using `fork()` and `execvp()`
* Background job execution with `&`
* Built-in `cd` command
* Automatic cleanup of background processes using a `SIGCHLD` handler
* Generate ASCII art banners from image files and optionally save them for future sessions
* Integrated terminal text editor with raw mode input, cursor navigation, and ANSI text styling

## Building

```bash
gcc shell.c -o shell
gcc textedit.c -o textedit
```

Place both executables in the same directory, as the shell launches the editor using `execvp()`.

## Usage

```bash
./shell
```

### Commands

| Command              | Description                        |
| -------------------- | ---------------------------------- |
| `cd <dir>`           | Change the current directory       |
| `help`               | Show built-in commands             |
| `textedit <file>`    | Open a file in the built-in editor |
| `banner <image>`     | Display an image as ASCII art      |
| `banner set <image>` | Save a startup banner              |
| `banner clear`       | Remove the saved banner            |
| `<command> &`        | Run a command in the background    |
| `exit`               | Exit the shell                     |

### Text Editor Shortcuts

| Key             | Action                          |
| --------------- | ------------------------------- |
| Arrow keys      | Move cursor                     |
| Enter           | Insert a new line               |
| Backspace       | Delete character or merge lines |
| Ctrl+S          | Save file                       |
| Ctrl+F          | Cycle text style                |
| Ctrl+R          | Cycle text color                |
| Ctrl+Q / Ctrl+C | Quit editor                     |

## Future Improvements

* Command history
* Tab completion
* Pipes and I/O redirection
* Syntax highlighting
* Scrolling for large files
* Line numbers and status bar
* Search functionality
* Basic job control (`fg` / `bg`)
