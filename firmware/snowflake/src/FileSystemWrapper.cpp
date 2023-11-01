#include "FileSystemWrapper.h"
#include "application.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

void dumpFilesAndDirs(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    Log.printf("%s:\r\n", path);
    while ((entry = readdir(dir)) != NULL) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        if (stat(fullpath, &fileStat) < 0) {
            closedir(dir);
            return;
        }

        if (S_ISDIR(fileStat.st_mode)) {
            Log.printf("drw-rw-rw- %10ld %s\r\n", fileStat.st_size, entry->d_name);
        } else {
            Log.printf("-rw-rw-rw- %10ld %s\r\n", fileStat.st_size, entry->d_name);
        }
    }

    closedir(dir);
}

void removeAllFiles(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    std::unique_ptr<char[]> fullpath(new char[PATH_MAX]);

    while ((entry = readdir(dir)) != NULL) {
        snprintf(fullpath.get(), PATH_MAX, "%s/%s", path, entry->d_name);
        if (stat(fullpath.get(), &fileStat) < 0) {
            return;
        }

        if (S_ISDIR(fileStat.st_mode)) {
            // Skip "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Check if it's one of the directories to keep
            if (strcmp(entry->d_name, "sys") == 0 || strcmp(entry->d_name, "usr") == 0 || strcmp(entry->d_name, "tmp") == 0) {
                continue;
            }

            // Recursively delete the directory
            removeAllFiles(fullpath.get());
            rmdir(fullpath.get()); // Delete the empty directory
        } else {
            // Delete regular files
            unlink(fullpath.get());
        }
    }

    closedir(dir);
}
