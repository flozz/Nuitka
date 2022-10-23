//     Copyright 2022, Kay Hayen, mailto:kay.hayen@gmail.com
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/sysctl.h>
#endif

#include "nuitka/filesystem_paths.h"

filename_char_t *getBinaryPath(void) {
    static filename_char_t binary_filename[MAXPATHLEN];

#if defined(_WIN32)
    DWORD res = GetModuleFileNameW(NULL, binary_filename, sizeof(binary_filename) / sizeof(wchar_t));
    if (res == 0) {
        abort();
    }
#elif defined(__APPLE__)
    uint32_t bufsize = sizeof(binary_filename);
    int res = _NSGetExecutablePath(binary_filename, &bufsize);

    if (res != 0) {
        abort();
    }
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
    /* Not all of FreeBSD has /proc file system, so use the appropriate
     * "sysctl" instead.
     */
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;
    size_t cb = sizeof(binary_filename);
    int res = sysctl(mib, 4, binary_filename, &cb, NULL, 0);

    if (res != 0) {
        abort();
    }
#else
    /* The remaining platforms, mostly Linux or compatible. */

    /* The "readlink" call does not terminate result, so fill zeros there, then
     * it is a proper C string right away. */
    memset(binary_filename, 0, sizeof(binary_filename));
    ssize_t res = readlink("/proc/self/exe", binary_filename, sizeof(binary_filename) - 1);

    if (res == -1) {
        abort();
    }
#endif

    return binary_filename;
}

bool readFileChunk(FILE_HANDLE file_handle, void *buffer, size_t size) {
    // printf("Reading %d\n", size);

#if defined(_WIN32)
    DWORD read_size;
    BOOL bool_res = ReadFile(file_handle, buffer, (DWORD)size, &read_size, NULL);

    return bool_res && (read_size == size);
#else
    size_t read_size = fread(buffer, 1, size, file_handle);

    return read_size == size;
#endif
}

bool writeFileChunk(FILE_HANDLE target_file, void *chunk, size_t chunk_size) {
#if defined(_WIN32)
    return WriteFile(target_file, chunk, (DWORD)chunk_size, NULL, NULL);
#else
    size_t written = fwrite(chunk, 1, chunk_size, target_file);
    return written == chunk_size;
#endif
}

FILE_HANDLE createFileForWriting(filename_char_t const *filename) {
#if defined(_WIN32)
    FILE_HANDLE result = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
#else
    FILE *result = fopen(filename, "wb");
#endif

    return result;
}

FILE_HANDLE openFileForReading(filename_char_t const *filename) {
#if defined(_WIN32)
    FILE_HANDLE result = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    FILE *result = fopen(filename, "rb");
#endif

    return result;
}

bool closeFile(FILE_HANDLE target_file) {
#if defined(_WIN32)
    CloseHandle(target_file);
    return true;
#else
    int r = fclose(target_file);

    return r == 0;
#endif
}

int64_t getFileSize(FILE_HANDLE file_handle) {
#if defined(_WIN32)
    // TODO: File size is truncated here, but maybe an OK thing.
    DWORD file_size = GetFileSize(file_handle, NULL);

    if (file_size == INVALID_FILE_SIZE) {
        return -1;
    }
#else
    int res = fseek(file_handle, 0, SEEK_END);

    if (res != 0) {
        return -1;
    }

    long file_size = ftell(file_handle);

    res = fseek(file_handle, 0, SEEK_SET);

    if (res != 0) {
        return -1;
    }
#endif

    return (int64_t)file_size;
}
