#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
//#include <linux/fb.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
#include <stdint.h>
//#include <linux/msm_mdp.h>
#include <string.h>
#include <sys/wait.h>


//int execute_and_capture(const char *filepath, char **buffer, size_t *buffer_size) {
int execute_and_capture(const char *filepath, char **buffer) {
    int pipefd[2];
    pid_t pid;
    int status;
    //size_t buffer_size = 0;
    
    // Initialize output buffer
    *buffer = NULL;
    
    // Create pipe for capturing output
    if (pipe(pipefd) == -1) {
        return -1;
    }
    
    // Fork the process
    pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    if (pid == 0) {
        // Child process
        // Close read end of pipe
        close(pipefd[0]);
        
        // Redirect stdout and stderr to pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        
        // Execute the file
        execl(filepath, filepath, NULL);
        
        // If we get here, exec failed
        printf("Failed to execute %s\n", filepath);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        // Close write end of pipe
        close(pipefd[1]);
        
        // Read output from pipe into buffer
        size_t total_read = 0;
        size_t buffer_capacity = 4096;
        char *temp_buffer = malloc(buffer_capacity);
        
        if (temp_buffer == NULL) {
            close(pipefd[0]);
            waitpid(pid, &status, 0);
            return -1;
        }
        
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], temp_buffer + total_read, 
                                  buffer_capacity - total_read)) > 0) {
            total_read += bytes_read;
            
            // Check if we need to grow buffer
            if (total_read == buffer_capacity) {
                buffer_capacity *= 2;
                char *new_buffer = realloc(temp_buffer, buffer_capacity);
                if (new_buffer == NULL) {
                    free(temp_buffer);
                    close(pipefd[0]);
                    waitpid(pid, &status, 0);
                    return -1;
                }
                temp_buffer = new_buffer;
            }
        }
        
        close(pipefd[0]);
        
        // Wait for child process to finish
        if (waitpid(pid, &status, 0) == -1) {
            free(temp_buffer);
            return -1;
        }
        
        // Check if child executed successfully
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            free(temp_buffer);
            return -1;
        }
        
        // Null-terminate the buffer
        if (total_read == buffer_capacity) {
            buffer_capacity++;
            char *new_buffer = realloc(temp_buffer, buffer_capacity);
            if (new_buffer == NULL) {
                free(temp_buffer);
                return -1;
            }
            temp_buffer = new_buffer;
        }
        temp_buffer[total_read] = '\0';
        
        // Return buffer to caller
        *buffer = temp_buffer;
 //       *buffer_size = total_read;
        
        return 0;
    }
}   


void hexdump(void *ptr, unsigned len)
{
    unsigned char *b = ptr;
    int count = 0;
    //printf("%s\n", (unsigned) b);
    printf("%x: ", (unsigned) b);
    while(len-- > 0) {
        printf("%02X ", *b++);
        if(++count == 16) {
            printf("\n%x: ", (unsigned) b);
            count = 0;
        }
    }
    if(count != 0) printf("\n");
}


void write_int(const char *FILEPATH, int timeout_ms)
{
    char str[20];
    int fd;
    int val;

    if (timeout_ms > 10000) timeout_ms = 1000;

    fd = open(FILEPATH, O_WRONLY);
    if (fd < 0)
        return;

    val = snprintf(str, sizeof(str), "%d", timeout_ms);
    write(fd, str, val);
    close(fd);

}

static int read_file(const char *path, char *buf, size_t sz)
{
    int fd;
    size_t cnt;
    fd = open(path, O_RDONLY, 0);
    if (fd < 0)
        goto err;
    cnt = read(fd, buf, sz - 1);
    if (cnt <= 0)
        goto err;
    buf[cnt] = '\0';
    if (buf[cnt - 1] == '\n') {
        cnt--;
        buf[cnt] = '\0';
    }
    close(fd);
    return cnt;
err:
    if (fd >= 0)
        close(fd);
    return -1;
}

int read_file_int(const char *path, int *val)
{
    char buf[32];
    int ret;
    int tmp;
    char *end;
    ret = read_file(path, buf, sizeof(buf));
    if (ret < 0)
        return -1;
    tmp = strtol(buf, &end, 0);
    if (end == buf ||
        ((end < buf+sizeof(buf)) && (*end != '\n' && *end != '\0')))
        goto err;
    *val = tmp;
    return 0;
err:
    return -1;
}
