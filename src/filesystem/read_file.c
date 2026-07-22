#include <liburing.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/terminal.h>
#include <string.h>

const char *read_file(const char *path, size_t *out_size){
    struct io_uring ring = {0};
    io_uring_queue_init(8, &ring, 0);

    int fd = open(path, O_RDONLY);
    if (fd <0){
        perror("open");
        return NULL;
    }

    struct stat filest;
    fstat(fd, &filest);
    if (filest.st_size <0){
        fprintf(stderr, "invalid filest.st_size (negative size)\n");
        return NULL;
    }
    size_t size = (unsigned)filest.st_size;

    char *buf = malloc(size + 1);
    if (!buf){
        perror("malloc");
        close(fd);
        io_uring_queue_exit(&ring);
        return NULL;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_read(sqe, fd, buf, size, 0);
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
    io_uring_wait_cqe(&ring, &cqe);
    if (cqe->res < 0){
        fprintf(stderr, "io_uring read failed\n");
        free(buf);
    }else{
        buf[cqe->res] = '\0';
        if (out_size) *out_size = cqe->res;
    }

    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);
    close(fd);

    char *content = strdup(buf);
    free(buf);
    return content;
}
