#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_NUM_OF_PROCESS "10"
#define CGROUP_FOLDER "/sys/fs/cgroup/pids/myCon/"
#define concat(a, b) (a "" b)

/* create files with the corresponding values */
void writeValue(const char *path, const char *value)
{
    /* syscall to open a file and return a file descriptor */
    int fp = open(path, O_WRONLY | O_APPEND);
    /* write value into the file */
    write(fp, value, strlen(value));
    /* close the file */
    close(fp);
}

/* create a cgroup setting some values */
void limitNumOfProcess()
{
    char pid[6];
    sprintf(pid, "%d", getpid());

    /* create the cgroup folder */
    mkdir(CGROUP_FOLDER, S_IRUSR | S_IWUSR);

     /* set max num of procs */
    writeValue(concat(CGROUP_FOLDER, "pids.max"), MAX_NUM_OF_PROCESS);
    
    /* enables automatic removal of abandoned cgroups.  */
    writeValue(concat(CGROUP_FOLDER, "notify_on_release"), "1"); 

    /*  move to current process to this cgroup */
    writeValue(concat(CGROUP_FOLDER, "cgroup.procs"), pid);
}

/* make some change within "our container" */
int child(void *args)
{
    /* clear the environment -> nice to have */
    clearenv();

    /* syscall for setting the hostname */
    if(sethostname("myCon", 5))
    {
        /* put some error handling */
        return EXIT_FAILURE;
    }
    
    /* syscall to change the current working directory */
    chdir("ubuntu-20.04");
    
    /* syscall to change root directory */
    if (chroot(".") < 0)
    {
        /* put some error handling */
        return EXIT_FAILURE;
    }

    /* jump to the new root folder */
    chdir("/");

    /* syscall to mount the new proc directory */
    if (mount("proc", "/proc", "proc", 0, 0))
    {
        /* put some error handling */
        return EXIT_FAILURE;
    }

    /* start a bash */
    system("bash");

    /* unmount the new proc directory */
    if (umount("/proc"))
    {
        /* put some error handling */
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main()
{
    /*  define mask for the child proccess (see https://man7.org/linux/man-pages/man2/clone.2.html) */
    int namespaces = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNET;

    /* function to limit the number of process within the container */
    limitNumOfProcess();

    // syscall for creating a child process (s. https://man7.org/linux/man-pages/man2/clone.2.html)
    pid_t p = clone(child, malloc(4096) + 4096, SIGCHLD | namespaces, NULL);
    if (p == -1)
    {
        // handle error
        return EXIT_FAILURE;
    }
    // wait for the child process to terminate
    waitpid(p, NULL, 0);
    return EXIT_SUCCESS;
}
