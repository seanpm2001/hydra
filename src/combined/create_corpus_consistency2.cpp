// SPDX-License-Identifier: MIT

/**
  create_corpus.cpp
Purpose: generate initial testing corpus for an image
Usage:
./create_corpus [image stat file] [corpus folder] [[mem]]
*/

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "Program.hpp"
#include "Image.hpp"
#include "SyscallMutator.hpp"
#include "Constants.hpp"

#define TEST_MEM_SIZE 8192
bool mem;

// Do open chmod fsync
void create_corpus(Image *image, char *folder)
{
    uint32_t cnt = 0;
    Program *program;

    for (FileObject *fobj : image->file_objs) {

        // TODO: we skip FIFO now
        if (fobj->type == I_FIFO)
            continue;

        program = new Program;
        program->avail_files = image->file_objs; // TODO: ad-hoc
        program->prepare_buffers();
        program->prepare_file_paths();

        OpenMutator *open_sm = new OpenMutator(program);
        open_sm->setTarget(open_sm->createTarget(fobj));
        int64_t fd_index = open_sm->getTarget()->ret_index;

        ChmodMutator *chmod_sm = new ChmodMutator(program);
        Syscall* syscall_chmod = NULL;

        syscall_chmod = chmod_sm->createTarget(ArgMap({{0, fd_index}}));
        chmod_sm->setTarget(syscall_chmod);

        std::string path;
        path = std::string(folder) + "/open_chmod_fsync" + std::to_string(cnt++);

        FsyncMutator *fsync_sm = new FsyncMutator(program);
        Syscall* syscall_fsync = NULL;
        syscall_fsync = fsync_sm->createTarget(ArgMap({{0, fd_index}}));
        if (syscall_fsync)
            fsync_sm->setTarget(syscall_fsync);

        if (!mem)
            program->serialize(path.c_str());
        else {
            char *buffer = (char *)malloc(TEST_MEM_SIZE);
            uint32_t new_len = program->serialize((uint8_t *)buffer, (uint32_t)TEST_MEM_SIZE);
            assert(new_len < TEST_MEM_SIZE && "too small TEST_MEM_SIZE");
            int fd = open(path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
            write(fd, buffer, new_len);
            close(fd);
            free(buffer);
        }

        fsync_sm->releaseTarget();
        delete fsync_sm;
        chmod_sm->releaseTarget();
        delete chmod_sm;
        open_sm->releaseTarget();
        delete open_sm;

        program->avail_files.clear();
        delete program;	
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        printf("usage: ./create_corpus_consistency2 istat/f2fs-10.istat DIR\n");
        return 1;
    }

    Image *image = Image::deserialize(argv[1]);
    mem = argc == 4;

    if (image)
        create_corpus(image, argv[2]);

    return 0;
}
